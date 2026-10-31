r"""THE DESK MEASUREMENTS (2026-10-06, frame generation phase 3, the build map's STAGE 1; docs\reports\fg-3\map-2026-10-06\
critic.md, section 4, and helper-wire.md, H0). TOOL CODE ONLY: nothing of the product is changed or needed beyond the
helper as built in this tree.

Frame generation runs in shadow: nothing AMD's library writes has reached a screen. Before a machine that SHOWS generated
frames is built, what the library writes is read at a desk, on the fake client against the real helper. The fake client's
mode is tools\test-fg-pipe\fgpipe_desk.h (--fake-fg=50; its head says what each item sends). It reads THE WHOLE generated
image back after every pair it measures, 3840x2160, through the game's own import code. THIS SCRIPT builds the fake client
under its own name (fgdesk.exe) in its own folder under %TEMP%, stages the development helper with AMD's three libraries
by their pins and NVIDIA's, runs one session an item (one helper, one fake client, NO WINDOW), holds every judgement and
prints every number. It stands on the cost bench (tools\test-fg-cost): that bench has what a pair COSTS, and from its
centre tiles that a reset dispatch is a copy of this frame's picture and that the pair after it interpolates. This tool
has what the image IS, over every pixel.

THE OUTCOMES, NAMED BEFORE THE RUN (written 2026-10-06, 22:15, before any session of this tool had run). For each item:
the question, what each answer would mean for the design, and the control that must fail.

1. THE RESET PAIR AND THE PAIRS AFTER IT (sessions R1D, R1Q; the map's X4, risk 5).
   a. What a reset dispatch writes. IF it is this frame's presented picture BIT FOR BIT (all 8,294,400 pixels, in every
      arm): the rule "a pair whose dispatch resets is fed, never shown" is a matter of pacing (the real frame shown half a
      period early, then again), not a guard against a wrong picture, and risk 5 is closed by measurement as well as by
      the rule. IF it is anything else (a blend, the frame before, a blank): the rule is the black-frame guard, and no
      path may ever let fgOut = 2n alone mean "show".
   b. The first pair after a reset (dispatch 1). It is WHOLE when, in every cycle of arms CUT, HIT and WH, its distance
      from the half-way picture over the whole image is at most twice the steady pairs' (dispatch 12 and later) or 0.005
      more, and its worst block of the 16 by 9 grid is at most twice the steady pairs' worst block or 0.01 more.
      IF WHOLE: a cut, a hitch and an engage's first pair can be written DISPATCH and not shown (helper-wire-13, rule 3),
      and each costs ONE generated frame. X4 closes as "one". IF NOT: such an event costs two (or as many as the curve
      says), and the budget of "2 non-cut withheld or late pairs per 300" is counted with that.
   c. Arm WH (today's kinds: the cut pair WITHHELD, the helper resets the next dispatch itself). IF its reset dispatch is
      a copy and its dispatch 1 is whole: today's kinds cost TWO pairs a cut (the withheld one and the reset one) where
      rule 3 costs one, and nothing else differs. IF the helper's own reset behaves otherwise than a told one: said.
   d. Arm ZV (every vector zero, the picture moving): what optical flow alone does, dispatch by dispatch. IF the distance
      falls between dispatch 1 and the steady pairs: for whatever the game gives no vectors for, the pairs right after a
      reset are worse than steady ones, by that much, and "whole" in b is whole only where the vectors are right.
      IF it does not fall: on this picture optical flow adds nothing a desk can see, and the question goes to the game.
   CONTROL (must fail): "a copy of this frame's, bit for bit" put to the steady dispatches, and "sits on the half-way
   picture (0.02 or nearer)" put to the reset dispatches. THE INSTRUMENT: this side's count of dispatches since a reset
   is the helper's own count (its cost rows), pair by pair.

2. THE GUIDES SMALLER THAN THE DISPLAY (sessions G2D, G2Q; helper-wire-8, helper-wire-4 b).
   The image at Quality guides is WHOLE when its inside (64 pixels in from every border) is 0.02 or nearer the half-way
   picture and no more than 0.01 farther than at DLAA guides, no quadrant is more than twice the nearest quadrant or
   0.005 more, and no block of the grid that does not touch a border is over 0.05.
   IF WHOLE: the sizes on the wire are right as they are and Quality needs no rule of its own. IF a quadrant or a border
   is wrong: a size or a scale is wrong somewhere between 'g', the prepare and the dispatch, and nothing is shown at a
   render scale until it is found. The four borders are printed for both sizes whatever the verdict: content that enters
   the picture has no source in the frame before, and what the library paints there is what a player sees at the edge.
   CONTROL (must fail): the same judgement on arm FN (every vector negated), at both sizes.

3. HDR (session H3; risk 2, helper-wire-7).
   a. Five constants (1, 4, 12.5, a 1000 nit red, a negative red) at four peaks on 'G' (1000, 400, 0 which the helper
      reads as 1000, and 80), fourteen still pairs each. IF every reading is the constant BIT FOR BIT: on a still picture
      a generated frame cannot differ in brightness from the real frames beside it, at any peak, and the peak rule stays
      as it is. IF a value above the peak comes back lower: the peak sent must be the larger of the display's peak and
      the encode's ceiling (helper-wire-7's rewrite). IF anything else differs: by how much and where is printed, and a
      still picture flickers at the real rate: the stop rule's subject, before any picture is shown.
   b. A still PICTURE (red to 12.3, a blue that is negative over half of it): the same question where values vary from
      pixel to pixel. IF exact: as a. IF not exact while the constants are: the library resamples or re-encodes, and the
      error's size against one code step says whether the eye can see it.
   c. Alpha. IF GEN's alpha is PRESENT's: the generated frame can be copied into the back buffer as it is. IF it is a
      constant, or the seed (not written at all): a copy of GEN carries that alpha into the back buffer, and whoever
      reads alpha after it (DXVK's presenter, a capture) must be checked before first light.
   CONTROL (must fail): the seed read back before 'G' is not the constant on any pixel; and the answer compared with a
   value one code step off in one channel differs on every pixel.

4. THE HUD RECTANGLE (sessions U4, U4E; risk 10, helper-wire-1, check-transport M3).
   a. Arm OWN, inside the rectangle. IF every pixel is THIS pair's colour exactly: the generated frame carries frame n's
      HUD unwarped, AMD reads HUDLESS at the dispatch (not at the configure, where the image still held the pair
      before), and no second Configure at 'G' is needed. IF it holds the colour of the pair before: Configure moves.
   b. The edge: how many pixels wide the band is where GEN is neither the rectangle nor the half-way picture.
   c. THE ONE-STEP PLANT. IF GEN there is the half-way picture as it is beside it (w within 0.02 of the unplanted
      place): a difference of one code step between HUDLESS and PRESENT is not taken for HUD, the absorb's 1-in-600
      frame need not be withheld for it, and risk a (speckle) is not a one-step risk. IF GEN there is the presented
      pixel: every pixel that differs by a step is held at the real rate, and the HUD-less picture must be bit equal.
      The five larger plants give the curve from "ignored" to "held".
   d. Arm STALE (HUDLESS is the pair before's picture). IF GEN is this frame's picture all over: a stale HUD-less picture
      turns the generated frame into a second copy of the real one (judder, not garbage).
   e. HUDLESS EMPTY (the helper's --fghud_empty=1): what the rectangle looks like when nothing says where the HUD is.
   CONTROL (must fail): "inside the rectangle every pixel of every reading is this pair's colour" put to arm SAME (the
   rectangle in both inputs, so nothing says it is HUD).

5. THE HELD ANSWER (session A5; section 5's third box, helper-wire-12).
   IF pair n's image is the same bit for bit 1.5 s later, behind the next frame, behind the next 'Q' (AMD's configure and
   prepare for n+1), behind the next copies and behind the next 'G' with fgIn withheld, and changes only behind fgIn:
   the game may read GEN(n) from fgOut = 2n until its OWN submit that signals fgIn for the next dispatched pair, no new
   fence is needed, and the rule is the map's one sentence. IF it changes at any earlier step: that step is the bound.
   THE QUEUE: a read submitted before the submit that signals fgIn, nothing waited, must hold pair n's image in every
   trial; a read submitted right behind it is counted (n's, n+1's or torn).
   CONTROL (must fail): the same comparison behind fgIn, and a read submitted after the next pair's answer.

6. TWO DEPTH PLANES (session Z6; helper-wire-3).
   IF the image with the planes swapped differs from the right one, and the right one is nearer the ideal at the object's
   edge: depth reaches the interpolation and a wrong plane shows at a desk; the numbers say where. IF swapped, flat and
   right are the same image: depth does not reach the picture here, a wrong plane CAN hide, and the game's depth must be
   proved some other way before a silhouette is trusted. The near plane on 'Q' at 0.05, 1.0 and 100: IF the image is the
   same, the value the helper passes cannot be judged by pixels at a desk.
   CONTROL (must fail): "the same image as the first arm" put to arm V (the object's vectors left out). And the first
   arm run again must give the first arm's image, or no difference between two arms means anything.

7. THE HELPER KILLED (sessions X7, X7a to X7f; LS-4, section 5's fourth box).
   IF PRESENT and HUDLESS read the same through the imports at once and 3 s after the helper is gone, and the product's
   order (acquire, copy, release) gives PRESENT's picture: the real frame can be fired from FG_PRESENT after a stop
   (LS-4's first form). IF NOT: once the transport has failed an armed pair's real frame is dropped and counted, or the
   real frame waits in a store of the game's own (the map's X5).
   The answer fence: IF it reads the top with the pair's value below it, a wait for "2n" returns and "at least 2n" says
   a frame; equality says none. That is the rule's whole case, measured.
   Killed with the dispatch in flight: what GEN holds (pair n's, pair n+1's whole, or neither) says what a "greater or
   equal" would have put on the screen.
   CONTROL (must fail): "the same image" put to PRESENT after a constant was written into it through the import.

8. K AT A DESK (sessions K8...; pgk-15 c). The distribution of 'G' written to the answer seen, at both guide sizes, in
   two orders, three takes each, the machine's load beside each. It is a FLOOR: no game on either queue. IF its 99th
   percentile at Quality is under about 3.0 ms (the room at 59 real frames a second by the map's one figure) and at DLAA
   is not: the first regime is Quality at the ceiling, as the cost entry says. IF the tail is long at a desk (the 99.9th
   several times the median with nothing else on the card): K's tail is the helper's or the driver's own and no pacing
   on the game's side removes it. Takes agree when their medians lie within 2 percent or 30 us and their 99th
   percentiles within 10 percent or 100 us; if they do not, each is printed alone and nothing is averaged.
   CONTROL (must fail): with the helper's lever --fgslow_d=20 (every 64th dispatch sleeps 20 ms first) "no pair over
   15 ms" must fail, and the pairs over 20 ms must be exactly the 34 the lever slept.

ADDED AFTER THE FIRST WHOLE RUN (2026-10-06, 23:10; the outcomes above stand as they were written before it):
  - Item 3's line was written as "bit for bit, or it flickers". The first run read differences of ONE CODE STEP, so the
    tables now carry the unit that judges them: the largest difference in code steps, the pixels more than one step
    off, and the largest difference of a pixel as a part of that pixel's own largest channel. helper-wire-7's own words
    were "exact or within one half-float step"; the report reads the numbers against both.
  - Arm WH's cycle is 17 pairs as the others' (18 shares a factor with the picture's six moves).
  - Item 1's table carries the distance from the even mix of the two frames: arm ZV sits on it for nine dispatches.
  - Item 8's table carries the lower quartile and K by the picture's move, and EVERY session is watched: another GPU
    user seen at any moment of it is named beside its numbers, and a take of item 8 that had one is NOT IDLE: it is
    printed, it is left out of the verdict on the takes, and it is run again.

  - TWO READINGS ADDED, THEIR OUTCOMES NAMED HERE BEFORE THEY RAN (23:20):
    HIGHLIGHTS IN MOTION AND THE PEAK (session G2H, and arms F80 and F0 of item 2: the same pairs with the peak on 'G'
    at 80 and at 0). Item 3's pictures stand still. IF the moving picture at 14 times its values (to 12.3, a 985 nit
    white) is as near its half-way picture as at its own values, and arm F80's image is arm F's bit for bit on every
    pair: highlights in motion are interpolated as they are, and the peak on 'G' does not reach the picture at all (the
    peak rule cannot cause a brightness step, whatever it sends). IF the peak changes the image: the peak sent must
    never be under the picture's own highest value, and the numbers say by how much it bends. Arm F0 (the helper reads
    0 as 1000) must be arm F bit for bit, or no difference between two arms means anything.
    WHICH INPUT A RESET DISPATCH COPIES (item 4 reads its arms' first pair too). IF the rectangle is there in arm OWN:
    the reset dispatch copies PRESENT, HUD and all, and a reset pair shown by mistake would be the real frame. IF it is
    not: it copies HUDLESS, and a reset pair shown by mistake would be a frame without its HUD.
    BY HOW MUCH THE PEAK BENDS THE PICTURE (added 2026-10-07, 00:00, after the first reading of arms F80 and F0: F0 was F
    bit for bit on every pair, F80 was not on 12, 33 and 27 of 36). Arm F's image of every second pair is kept and the
    other arms' images of the same pairs are compared with it pixel by pixel. IF the largest difference is a code step
    or two: the peak reaches the arithmetic and not the eye, and the peak rule stays a matter of AMD's header. IF it
    is more than about half a percent of a pixel's value anywhere away from the borders: a generated frame's levels
    depend on the peak sent, and the peak must be the display's own and must never change inside a session.
    WHERE ARM ZV'S CURVE ENDS (arm ZL: as ZV with an event every 49th pair, read to dispatch 48). The first whole run
    read ZV on the even mix of the two frames for nine dispatches and off it from the tenth, still moving at the
    sixteenth, where its cycle ended. IF ZL settles: that dispatch is where a pair is steady FOR THE PICTURE where the
    vectors are wrong, and the cost bench's "steady from the 12th" is cost's rule only. IF it never settles in 48: said.
  - The 30 minutes of waiting for the card are per spell of somebody else's use, not per session: 31 sessions behind
    one long bench would otherwise wait fifteen hours. After 30 minutes of one spell the sessions go on, and say so.
    And the card counts as free only when nobody has held it for eight seconds: another tool's runner leaves a few
    seconds between two of its sessions, and this tool's first runs slipped into those gaps.

WIRE 2 (2026-10-07, 02:20: the main line's one revision of the wire merged in, the mode ported to it). Every session asks
for its set WITH the HUD-less image explicitly (--fake-desk-hudless=1: the 'g' flag clear), as every number of the run
of record was measured. ONE SESSION IS NEW, R1N: item 1 on THE GAME'S SHAPE since wire 2, a set asked WITHOUT the
HUD-less image (two images, PRESENT alone copied). Its outcome, named before it ran: IF its reset dispatches are this
frame's picture bit for bit and its dispatch 1 is whole by item 1's own line, item 1's two answers (a reset pair is the
real frame; a cut costs one generated frame) hold on the shape the game runs. IF NOT: the difference is printed, and
the two answers are the three-image set's only. THE PIXEL ANSWERS OF THE OTHER SESSIONS MUST BE THE RUN OF RECORD'S
(docs\reports\fg-3\desk-2026-10-06\desk-run-2026-10-07.txt): the wire changed, the pictures did not.

WHAT A DESK CANNOT SHOW (said in the report too): a real picture (the head), a jittered scene, a game on the two queues,
another vendor's hand-off of an image between two processes, and what a screen does with a frame.

ONE GPU USER AT A TIME. Before every session this script lists the processes named dlssiv-helper.exe, fgpipe*.exe,
fgcost*.exe, dt_fg_test*.exe and perfproof*.exe that it did not start: while one runs it waits, 30 s at a time, for up
to 30 minutes, then goes on and says so beside that session's numbers.
NOTHING IS LEFT BEHIND: a session that does not end is ended BY ITS OWN PROCESS, and at the end every process whose image
is under this tool's own folder is ended by its pid. Nothing is ever ended by name.

    python tools\test-fg-desk\run.py                 every session (about 20 minutes)
    python tools\test-fg-desk\run.py --only=H3,A5    the named sessions
    python tools\test-fg-desk\run.py --item=8        one item's sessions
    ... --keep                                       do not empty this tool's folder first
    ... --read                                       run nothing: read what the folder holds and print the tables
exit code 0 = the instrument held and every control held. The last line is PASS or FAIL. What it printed is kept as
result.txt in its folder. NOT part of tools\prove-all.py: it takes the card for minutes.
"""
import ctypes, ctypes.wintypes as wt, datetime, os, re, shutil, subprocess, sys, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import ffx_pin

OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-desk")
HELPER = os.path.join(ROOT, "host", "dlssiv-helper.exe")
NGX = os.path.join(ROOT, "external", "ngx", "nvngx_dlss.dll")
ARGS = sys.argv[1:]
READ = "--read" in ARGS
KEEP = "--keep" in ARGS or READ
ONLY = next((a.split("=", 1)[1].split(",") for a in ARGS if a.startswith("--only=")), None)
ITEM = next((int(a.split("=", 1)[1]) for a in ARGS if a.startswith("--item=")), None)
NOWIN = subprocess.CREATE_NO_WINDOW
PIXELS = 3840 * 2160
STEADY = 12               # a dispatch is steady from the 12th after one that reset (the cost bench's rule, from AMD's source)
K_PAIRS = 2200            # timed pairs a take of item 8
K_WARM = 600              # pairs before them that are nobody's (fgpipe_desk.h, DeskK)
SLOW_MS = 20              # item 8's control: the helper's --fgslow_d
KILL_US = (500, 1000, 1500, 2000, 2500, 3500)   # item 7: the helper killed this long after 'G' was written, the dispatch in flight
COMPILERS = ("cl.exe", "link.exe", "cmake.exe", "ninja.exe", "msbuild.exe", "fxc.exe", "dxc.exe")
CARD_NAMES = ("fgpipe", "fgcost", "fgdesk", "dt_fg_test", "perfproof")

passes, fails, answers, shared = [], [], [], []
spell = [0]               # seconds waited in the present spell of somebody else's use of the card


class Tee:
    def __init__(self, path): self.f = open(path, "w", encoding="utf-8"); self.o = sys.stdout
    def write(self, s): self.o.write(s); self.f.write(s)
    def flush(self): self.o.flush(); self.f.flush()


def proof(ok, sid, text):
    (passes if ok else fails).append((sid, text))
    print("  %s  [%s] %s" % ("PASS" if ok else "FAIL", sid, text))
    return ok


def answer(sid, text):
    answers.append((sid, text))
    print("  ANSWER  [%s] %s" % (sid, text))


# ---------------------------------------------------------------------------------------------------------------- the machine
# (as the cost bench's runner reads it: tools\test-fg-cost\run.py)
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


def processes3():
    """[(pid, image name, parent's pid)] of every process, by a snapshot (no name is ever acted on: see ours())."""
    out = []
    snap = k32.CreateToolhelp32Snapshot(2, 0)
    if not snap or snap == wt.HANDLE(-1).value: return out
    e = PE32(); e.dwSize = ctypes.sizeof(PE32)
    ok = k32.Process32FirstW(snap, ctypes.byref(e))
    while ok:
        out.append((e.th32ProcessID, e.szExeFile, e.th32ParentProcessID))
        ok = k32.Process32NextW(snap, ctypes.byref(e))
    k32.CloseHandle(snap)
    return out


def processes():
    return [(pid, name) for pid, name, _ in processes3()]


def image_of(pid):
    h = k32.OpenProcess(0x1000, False, pid)             # PROCESS_QUERY_LIMITED_INFORMATION
    if not h: return ""
    buf = ctypes.create_unicode_buffer(1024); n = wt.DWORD(1024)
    ok = k32.QueryFullProcessImageNameW(h, 0, buf, ctypes.byref(n))
    k32.CloseHandle(h)
    return buf.value if ok else ""


def under_out(pid):
    return os.path.normcase(image_of(pid)).startswith(os.path.normcase(os.path.abspath(OUT)) + os.sep)


def ours():
    """The pids of the processes whose image is under THIS TOOL'S OWN FOLDER: the fake client and the helper it started."""
    return [(pid, name) for pid, name in processes() if name.lower() in ("fgdesk.exe", "dlssiv-helper.exe") and under_out(pid)]


def end_ours(why):
    left = ours()
    for pid, name in left:
        h = k32.OpenProcess(1, False, pid)              # PROCESS_TERMINATE
        if h:
            k32.TerminateProcess(h, 9); k32.CloseHandle(h)
        print("  ended by its pid (%s): %s, pid %d" % (why, name, pid))
    return left


def card_users(own=None):
    """The programs that hold a D3D device and are NOT this tool's: the helper wherever it runs from, and the desk tools by
    their names. [(pid, name, image)]. `own`: the pid of this tool's own fake client while a session runs: it and the helper
    it started are never counted, whatever their image reads as (a process that is starting or ending has no readable
    image for a moment, and the first form of the watch took this tool's own fake client for a stranger twice)."""
    out = []
    for pid, name, ppid in processes3():
        low = name.lower()
        if low == "dlssiv-helper.exe" or (low.endswith(".exe") and low.startswith(CARD_NAMES)):
            if own is not None and (pid == own or ppid == own): continue
            if not under_out(pid): out.append((pid, name, image_of(pid)))
    return out


def wait_for_card(sid):
    """ONE GPU USER AT A TIME: wait while another one runs, 30 s at a time, 30 minutes at most. Seconds waited.
    THE CARD IS FREE WHEN NOBODY HAS HELD IT FOR EIGHT SECONDS, looked at every second: another tool's runner leaves a few
    seconds between two of its sessions, and a session of this tool started in that gap would run on top of its next one
    (seen on 2026-10-06: this tool's first runs slipped into a cost bench's gaps)."""
    waited = 0
    while True:
        u = card_users()
        for _ in range(8):
            if u: break
            time.sleep(1.0); u = card_users()
        if not u:
            spell[0] = 0
            return waited
        if waited == 0 and spell[0] < 1800: print("  another program has the card (%s): waiting, 30 s at a time, 30 minutes at most" % ", ".join("%s pid %d" % (n, p) for p, n, _ in u))
        if spell[0] >= 1800:
            print("  STILL THERE AFTER 30 MINUTES OF WAITING (%s): session %s goes on beside it" % (", ".join(n for _, n, _ in u), sid))
            shared.append(sid)
            return waited
        time.sleep(30); waited += 30; spell[0] += 30


def game_runs():
    """Is THE GAME running (a process named GTAIV.exe whose image is a megabyte or more: the kit's tests run ping.exe under that name)."""
    for pid, name in processes():
        if name.lower() != "gtaiv.exe": continue
        img = image_of(pid)
        try: size = os.path.getsize(img) if img else -1
        except OSError: size = -1
        if not (0 <= size < (1 << 20)): return True
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


# ---------------------------------------------------------------------------------------------------------------- the build
def vcvars(bits):
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"], creationflags=NOWIN).decode().strip()
    return os.path.join(ip, r"VC\Auxiliary\Build\vcvars%d.bat" % bits)


def prepare():
    """The fake client built under this tool's own name, and a folder with the helper and the libraries. The folder, or None."""
    if not os.path.exists(HELPER):
        proof(False, "build", "%s is not built (build-host.ps1, no flags)" % HELPER); return None
    blob = open(HELPER, "rb").read()
    if b" development, backend" not in blob or b"fghud_empty" not in blob or b"fgslow_d" not in blob:
        proof(False, "build", "host\\dlssiv-helper.exe is not a development build of this tree (its levers --fghud_empty and --fgslow_d are needed): build-host.ps1 with no flags"); return None
    bad = ffx_pin.cross_checks()
    if bad:
        for w in bad: print("  " + w)
        proof(False, "build", "the pins of AMD's libraries disagree with the packager's or with VERSION.txt"); return None
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /DWIN32 /I"%s" /I"%s" /I"%s" "%s" /Fe:fgdesk.exe /Fo:fgdesk.obj /link /INCREMENTAL:NO'
          % (vcvars(32), os.path.join(ROOT, "external", "vulkan"), os.path.join(ROOT, "source", "transport"), os.path.join(ROOT, "source", "fg"),
             os.path.join(ROOT, "tools", "test-fg-pipe", "fgpipe.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT, creationflags=NOWIN)
    if not proof(r.returncode == 0 and "warning C" not in r.stdout, "build", "the fake client compiles cleanly, 32-bit, under this tool's own name (fgdesk.exe)"):
        print(r.stdout[-3000:]); return None
    d = os.path.join(OUT, "full")
    os.makedirs(d, exist_ok=True)                       # (with --keep the folder is there: the helper is staged again, a library that is there stays)
    shutil.copy2(HELPER, os.path.join(d, "dlssiv-helper.exe"))
    for n in (ffx_pin.LOADER, ffx_pin.UPSCALER, ffx_pin.FRAMEGEN):
        why = ffx_pin.check(n)
        if why:
            proof(False, "build", "%s: a library nobody has checked is not staged" % why); return None
        if os.path.exists(os.path.join(d, n)): continue
        try: os.link(os.path.join(ffx_pin.FFX, n), os.path.join(d, n))
        except OSError: shutil.copy2(os.path.join(ffx_pin.FFX, n), os.path.join(d, n))
    if not os.path.exists(os.path.join(d, os.path.basename(NGX))): shutil.copy2(NGX, os.path.join(d, os.path.basename(NGX)))
    print("the development helper as built in this tree, AMD's three libraries by their pins; the sessions run under %s" % OUT)
    return d


# ---------------------------------------------------------------------------------------------------------------- the sessions
def P(sid, name): return os.path.join(OUT, "%s-%s" % (sid, name))


def plan():
    """[(id, item, what it is, the helper's arguments, the fake client's, with the helper's cost rows)]."""
    s = []
    gname = {"D": "DLAA guides (3840x2160)", "Q": "Quality guides (2560x1440)"}
    for g, gi in (("D", 0), ("Q", 1)):
        s.append(("R1" + g, 1, "ITEM 1, the reset pair and the pairs after it, " + gname[g], [], ["--fake-desk-item=1", "--fake-desk-guides=%d" % gi], True))
    s.append(("R1N", 1, "ITEM 1 on THE GAME'S SHAPE (wire 2): a set asked WITHOUT the HUD-less image, DLAA guides", [], ["--fake-desk-item=1", "--fake-desk-guides=0", "--fake-desk-hudless=0"], True))
    for g, gi in (("D", 0), ("Q", 1)):
        s.append(("G2" + g, 2, "ITEM 2, steady pairs, the whole image by place, " + gname[g], [], ["--fake-desk-item=2", "--fake-desk-guides=%d" % gi], True))
    s.append(("G2H", 2, "ITEM 2 at HDR levels: steady pairs of the picture at 14 times its values (to 12.3), DLAA guides", [], ["--fake-desk-item=2", "--fake-desk-guides=0", "--fake-desk-scale=14"], True))
    s.append(("H3", 3, "ITEM 3, HDR: constants, alpha and a still picture by bits; then an 8-bit set", [], ["--fake-desk-item=3"], False))
    s.append(("U4", 4, "ITEM 4, the HUD's rectangle, HUDLESS as the game would send it (three arms)", [], ["--fake-desk-item=4"], False))
    s.append(("U4E", 4, "ITEM 4, the HUD's rectangle, HUDLESS EMPTY (the helper's --fghud_empty=1)", ["--fghud_empty=1"], ["--fake-desk-item=4"], False))
    s.append(("A5", 5, "ITEM 5, the held answer, by pixels", [], ["--fake-desk-item=5"], False))
    s.append(("Z6", 6, "ITEM 6, two depth planes", [], ["--fake-desk-item=6"], False))
    s.append(("X7", 7, "ITEM 7, the helper killed with the pending dispatch HELD behind fgIn", [], ["--fake-desk-item=7"], False))
    for i, us in enumerate(KILL_US):
        s.append(("X7" + "abcdef"[i], 7, "ITEM 7, the helper killed %d us after 'G' was written, the dispatch IN FLIGHT" % us, [], ["--fake-desk-item=7", "--fake-desk-killus=%d" % us], False))
    for g, gi in (("D", 0), ("Q", 1)):
        for order in (0, 1):
            for take in "abc":
                s.append(("K8%s%d%s" % (g, order, take), 8, "ITEM 8, K at a desk, %s, %s, take %s" % (gname[g], "the game's order" if order else "every part waited for", take), [],
                          ["--fake-desk-item=8", "--fake-desk-guides=%d" % gi, "--fake-desk-order=%d" % order, "--fake-desk-pairs=%d" % K_PAIRS], False))
        s.append(("K8%s0w" % g, 8, "ITEM 8, K at a desk, %s, every part waited for, the answer seen by a WAIT" % gname[g], [],
                  ["--fake-desk-item=8", "--fake-desk-guides=%d" % gi, "--fake-desk-order=0", "--fake-desk-poll=0", "--fake-desk-pairs=%d" % K_PAIRS], False))
    s.append(("K8C", 8, "ITEM 8, THE CONTROL: DLAA guides, every part waited for, the helper's --fgslow_d=%d" % SLOW_MS, ["--fgslow_d=%d" % SLOW_MS],
              ["--fake-desk-item=8", "--fake-desk-guides=0", "--fake-desk-order=0", "--fake-desk-pairs=%d" % K_PAIRS], False))
    return [x for x in s if (ONLY is None or x[0] in ONLY) and (ITEM is None or x[1] == ITEM)]


def session(d, sid, item, title, helper_args, fake_args, marks):
    """One helper and one fake client, to its end or to the deadline. What it left is read by read_session()."""
    print("SESSION %s: %s" % (sid, title))
    waited = wait_for_card(sid)
    i0, a0 = cpu_times(); time.sleep(3.0); i1, a1 = cpu_times()
    before = 100.0 * (1.0 - (i1 - i0) / max(1, a1 - a0)); comp0 = compilers_now(); game = game_runs()
    print("  the machine: CPU %.0f%% busy over 3 s; compilers running: %s%s" % (before, comp0, "; THE GAME IS RUNNING" if game else ""))
    for name in ("rows.txt", "helper.csv", "helper.log", "out.txt", "rc.txt", "smi.csv"):
        if os.path.exists(P(sid, name)): os.remove(P(sid, name))
    args = ["--fake-fg=50", "--fake-want-fg=3", "--fake-desk-out=" + P(sid, "rows.txt")] + list(fake_args) + ["--fg=1"] + list(helper_args)
    if not any(a.startswith("--fake-desk-hudless=") for a in args): args.insert(3, "--fake-desk-hudless=1")   # wire 2: a set WITH the HUD-less image, asked explicitly
    if marks: args += ["--gpu_probe=1", "--fgcost_csv=" + P(sid, "helper.csv")]
    smi = Smi(P(sid, "smi.csv")) if item == 8 else None
    if smi: time.sleep(1.2)
    i0, a0 = cpu_times(); t0 = time.time()
    p = subprocess.Popen([os.path.join(OUT, "fgdesk.exe"), os.path.join(d, "dlssiv-helper.exe"), P(sid, "helper.log")] + args,
                         stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=d, creationflags=NOWIN)
    stop = threading.Event(); seen = {}

    def watch():                                        # another GPU user at any moment of the session, by name and pid
        while not stop.wait(1.5):
            for q, n, _ in card_users(p.pid): seen["%s pid %d" % (n, q)] = 1
    threading.Thread(target=watch, daemon=True).start()
    try:
        out, _ = p.communicate(timeout=900)
        rc = p.returncode
    except subprocess.TimeoutExpired:
        p.kill(); out, _ = p.communicate(); rc = 99
        out = (out or "") + "\n  FAIL  the fake client did not finish in 900 s and was ended by its own process\nFAIL\n"
    took = time.time() - t0
    stop.set()
    i1, a1 = cpu_times(); comp1 = compilers_now()
    for q, n, _ in card_users(): seen["%s pid %d" % (n, q)] = 1
    if smi:
        time.sleep(1.2); smi.stop()
    left = [x for x in ours()]
    if left: end_ours("left after session %s" % sid)
    open(P(sid, "out.txt"), "w", encoding="utf-8").write(out)
    open(P(sid, "rc.txt"), "w").write("%d\n%d\n%.1f\n%s\n%.1f\n%s\n%d\n%d\n%.1f\n%s\n" % (rc, len(left), before, comp0, 100.0 * (1.0 - (i1 - i0) / max(1, a1 - a0)), comp1, 1 if game else 0, waited, took,
                                                                             "; ".join(sorted(seen))))
    return True


def read_rows(path):
    """[(tag, {key: text})] from a rows file: a tag, then key=value pairs."""
    rows = []
    if not os.path.exists(path): return rows
    for l in open(path, encoding="utf-8", errors="replace"):
        l = l.strip()
        if not l: continue
        tag = l.split(" ", 1)[0]
        rows.append((tag, dict(m.group(1, 2) for m in re.finditer(r"(\w+)=(\S+)", l))))
    return rows


def read_session(sid):
    if not os.path.exists(P(sid, "out.txt")) or not os.path.exists(P(sid, "rc.txt")): return None
    s = {"sid": sid}
    s["out"] = open(P(sid, "out.txt"), encoding="utf-8", errors="replace").read()
    rc = open(P(sid, "rc.txt")).read().split("\n")
    s["rc"] = int(rc[0]); s["left"] = int(rc[1])
    # (a session's own fake client is never "another GPU user": its pid is on its first line. See card_users.)
    me = re.search(r"the fake client: 32-bit, pid (\d+)", s["out"])
    others = "; ".join(x for x in rc[9].split("; ") if x and not (me and x == "fgdesk.exe pid %s" % me.group(1)))
    s["machine"] = {"cpu": float(rc[2]), "compilers": rc[3], "during": float(rc[4]), "after": rc[5], "game": rc[6] == "1", "waited": int(rc[7]), "took": float(rc[8]), "others": others}
    s["log"] = open(P(sid, "helper.log"), encoding="utf-8", errors="replace").read() if os.path.exists(P(sid, "helper.log")) else ""
    lines = s["out"].splitlines()
    s["passed"] = [l for l in lines if l.startswith("  PASS  ")]; s["failed"] = [l for l in lines if l.startswith("  FAIL  ")]
    s["last"] = lines[-1].strip() if lines else ""
    s["rows"] = read_rows(P(sid, "rows.txt"))
    s["helper"] = {}
    if os.path.exists(P(sid, "helper.csv")):
        with open(P(sid, "helper.csv"), encoding="utf-8", errors="replace") as f:
            head = f.readline().strip().split(",")
            for l in f:
                q = l.strip().split(",")
                if len(q) == len(head):
                    r = dict(zip(head, q))
                    if r["kind"] == "D": s["helper"][int(r["n"])] = r
    s["smi"] = read_smi(P(sid, "smi.csv"))
    return s


def load_words(m):
    return ("CPU %.0f%% busy over 3 s before it and %.0f%% over the whole of it (this tool's two processes among that); compilers before: %s, at its end: %s%s%s%s"
            % (m["cpu"], m["during"], m["compilers"], m["after"], "; THE GAME WAS RUNNING" if m["game"] else "",
               ("; waited %d s for the card" % m["waited"]) if m["waited"] else "", ("; ANOTHER GPU USER DURING IT: " + m["others"]) if m["others"] else ""))


def instrument(s):
    """The fake client's own verdict on a session: it ended PASS, by itself, with nothing left behind."""
    ok = s["rc"] == 0 and s["last"] == "PASS" and not s["failed"] and not s["left"]
    proof(ok, s["sid"], "the session ran to its end: the fake client's %d proofs passed, %d failed, exit code %d, last line %s, %d process(es) left; %.0f s%s"
          % (len(s["passed"]), len(s["failed"]), s["rc"], s["last"], s["left"], s["machine"]["took"], "" if ok else "; " + "; ".join(l.strip()[:200] for l in s["failed"][:3])))
    return ok


# ---------------------------------------------------------------------------------------------------------------- arithmetic
def pct(v, p):
    v = sorted(v)
    return v[int(p * (len(v) - 1))] if v else None


def mean(v):
    v = list(v)
    return sum(v) / len(v) if v else None


def f3(x): return "none" if x is None else "%.4f" % x


def span(v, fmt="%.4f"):
    v = [x for x in v if x is not None]
    if not v: return "none"
    return (fmt + " (" + fmt + " to " + fmt + ", %d)") % (mean(v), min(v), max(v), len(v))


def moves(s, tag="desk-move"):
    """A session's readings of the moving picture, numbers made numbers."""
    out = []
    for t, r in s["rows"]:
        if t != tag: continue
        d = dict(r)
        for k in ("mid", "cur", "prev", "mix", "imid", "icur", "bt", "bb", "bl", "br", "q0", "q1", "q2", "q3", "amid", "achange", "maxmid", "wblk", "amin", "amax", "amean"): d[k] = float(d[k])
        for k in ("cyc", "k", "n", "move", "told", "eqcur", "eqprev", "eqmid", "notgrey", "nonfinite", "a1", "a0", "ax", "wblkat", "ring", "far", "ared", "acur"): d[k] = int(d[k])
        d["over"] = [int(x) for x in d["over"].split("/")]; d["blk"] = [float(x) for x in d["blk"].split(";")]
        out.append(d)
    return out


def alpha_words(rows):
    """What alpha holds over several readings of the moving picture, in words."""
    if not rows: return "none"
    return ("1.0 on %d to %d pixels, 0 on %d to %d, another value on %d to %d (values from %.4f to %.4f, mean %.4f); the generated red's own bits on %d to %d pixels, this frame's red's bits on %d to %d"
            % (min(r["a1"] for r in rows), max(r["a1"] for r in rows), min(r["a0"] for r in rows), max(r["a0"] for r in rows), min(r["ax"] for r in rows), max(r["ax"] for r in rows),
               min(r["amin"] for r in rows), max(r["amax"] for r in rows), sum(r["amean"] for r in rows) / len(rows), min(r["ared"] for r in rows), max(r["ared"] for r in rows),
               min(r["acur"] for r in rows), max(r["acur"] for r in rows)))


def inner_blocks(blk):
    """The blocks of the 16 by 9 grid that touch no border."""
    return [blk[y * 16 + x] for y in range(1, 8) for x in range(1, 15)]


# ---------------------------------------------------------------------------------------------------------------- item 1
def item1(S):
    for sid, gname in (("R1D", "DLAA guides"), ("R1Q", "Quality guides"), ("R1N", "DLAA guides, THE GAME'S SHAPE: A SET WITHOUT THE HUD-LESS IMAGE")):
        s = S.get(sid)
        if not s: continue
        print("\nITEM 1, THE RESET PAIR AND THE PAIRS AFTER IT, %s (session %s; %s)" % (gname, sid, load_words(s["machine"])))
        if not instrument(s): continue
        want = "two" if sid == "R1N" else "three"
        made = re.findall(r"frame generation: created at [^\n]*?(two|three) images", s["log"])
        proof(bool(made) and all(w == want for w in made) and ("images=%d" % (2 if sid == "R1N" else 3)) in s["out"], sid,
              "the set is the shape this session asked for: the helper's log says it created %s, and this side imported %s" % (", ".join(w + " images" for w in made) or "nothing", "two" if "images=2" in s["out"] else "three" if "images=3" in s["out"] else "?"))
        m = [r for r in moves(s) if r["item"] == "1"]
        print("  the whole generated image (%d pixels) behind every dispatch, by the dispatch's count since the one that reset (0 is the reset dispatch). Distances are parts of the" % PIXELS)
        print("  change between the two presented pictures: 0 is that picture exactly, 1 is as far as the two frames are from each other. Mean over the cycles (least to largest, cycles).")
        steady_by_arm = {}
        for arm, what in (("CUT", "the 'Q' told a cut, the picture JUMPS (rule 3: a cut written DISPATCH)"), ("HIT", "the 'Q' told a reset in steady motion, no jump (a hitch)"),
                          ("WH", "today's kinds: the cut pair WITHHELD, the helper resets the next dispatch itself"), ("ZV", "as CUT with EVERY VECTOR ZERO: optical flow alone"),
                          ("ZL", "as ZV with an event every 49th pair: where the curve ends")):
            rows = [r for r in m if r["arm"] == arm]
            if not rows: continue
            print("  arm %s, %s:" % (arm, what))
            print("    k   from the half-way picture       its inside   worst block of 144   from this frame's   from the previous   from their even mix   largest pixel   pixels over 0.05   of them 64 px or more inside   bit for bit this frame's")
            for k in range(0, 49):
                rk = [r for r in rows if r["k"] == k]
                if not rk: continue
                print("    %-3d %-31s %-12s %-20s %-19s %-19s %-21s %-15s %-18s %-30s %s" % (
                    k, span([r["mid"] for r in rk]), f3(mean(r["imid"] for r in rk)), f3(mean(r["wblk"] for r in rk)), f3(mean(r["cur"] for r in rk)), f3(mean(r["prev"] for r in rk)), f3(mean(r["mix"] for r in rk)),
                    "%.4f" % max(r["maxmid"] for r in rk), "%d to %d" % (min(r["over"][2] for r in rk), max(r["over"][2] for r in rk)),
                    "%d to %d" % (min(r["far"] for r in rk), max(r["far"] for r in rk)), "%d to %d" % (min(r["eqcur"] for r in rk), max(r["eqcur"] for r in rk))))
            print("    alpha behind the reset dispatches: %s" % alpha_words([r for r in rows if r["k"] == 0]))
            print("    alpha behind the steady dispatches: %s" % alpha_words([r for r in rows if r["k"] >= STEADY]))
            steady_by_arm[arm] = [r for r in rows if r["k"] >= STEADY]
        # ---- the instrument: this side's count is the helper's
        if s["helper"]:
            bad = [(r["n"], r["k"], s["helper"][r["n"]]["since"]) for r in m if r["n"] in s["helper"] and int(s["helper"][r["n"]]["since"]) != r["k"]]
            seen = sum(1 for r in m if r["n"] in s["helper"])
            proof(seen == len(m) and not bad, sid, "THE INSTRUMENT: this side's count of dispatches since a reset is the helper's own for %d of %d readings (its cost rows, by the pair's number)%s"
                  % (seen - len(bad), len(m), "" if not bad else "; the first that differs: pair %d, here %d, the helper %s" % bad[0]))
        else:
            proof(False, sid, "THE INSTRUMENT: the helper's cost rows were not written: this side's count of dispatches since a reset has nothing to be checked against")
        # ---- a. the reset dispatch
        r0 = [r for r in m if r["k"] == 0]
        exact = [r for r in r0 if r["eqcur"] == PIXELS]
        answer(sid, "1a. A RESET DISPATCH WRITES %s: %d of %d reset dispatches (arms %s) are this frame's presented picture bit for bit in the three colours over all %d pixels%s; alpha reads 1.0 on %d to %d pixels"
               % ("A COPY OF THIS FRAME'S PRESENTED PICTURE, BIT FOR BIT" if r0 and len(exact) == len(r0) else "NOT A BIT-EXACT COPY OF THIS FRAME'S PICTURE", len(exact), len(r0),
                  ", ".join(sorted(set(r["arm"] for r in r0))), PIXELS,
                  "" if len(exact) == len(r0) else "; the least: %d pixels, the box of the others %s, %.4f from this frame's" % (min(r["eqcur"] for r in r0), min(r0, key=lambda r: r["eqcur"])["box"], max(r["cur"] for r in r0)),
                  min(r["a1"] for r in r0) if r0 else 0, max(r["a1"] for r in r0) if r0 else 0))
        # ---- the control: the two judgements put to the wrong pairs
        st = [r for r in m if r["k"] >= STEADY and r["arm"] not in ("ZV", "ZL")]
        c1 = [r for r in st if r["eqcur"] == PIXELS]; c2 = [r for r in r0 if r["mid"] <= 0.02]
        proof(st and r0 and not c1 and not c2, sid, "THE CONTROL: \"a copy of this frame's, bit for bit\" FAILS on all %d steady dispatches (the most pixels that are this frame's: %d of %d), and \"0.02 or nearer the half-way picture\" FAILS on all %d reset dispatches (the nearest: %.4f)"
              % (len(st), max(r["eqcur"] for r in st) if st else 0, PIXELS, len(r0), min(r["mid"] for r in r0) if r0 else 0))
        # ---- b and c. the first pair after a reset
        for arm in ("CUT", "HIT", "WH"):
            rows = [r for r in m if r["arm"] == arm]; first = [r for r in rows if r["k"] == 1]; sd = steady_by_arm.get(arm, [])
            if not first or not sd: continue
            sm, sw = mean(r["mid"] for r in sd), mean(r["wblk"] for r in sd)
            whole = all(r["mid"] <= max(2 * sm, sm + 0.005) and r["wblk"] <= max(2 * sw, sw + 0.01) for r in first)
            answer(sid, "1%s. arm %s: THE FIRST PAIR AFTER A RESET IS %s: %s from the half-way picture over the whole image against the steady pairs' %s; its worst block %s against %s; its largest pixel %.4f against %.4f; pixels over 0.05: %d to %d against %d to %d"
                   % ("c" if arm == "WH" else "b", arm, "WHOLE" if whole else "NOT WHOLE", span([r["mid"] for r in first]), span([r["mid"] for r in sd]), span([r["wblk"] for r in first]), span([r["wblk"] for r in sd]),
                      max(r["maxmid"] for r in first), max(r["maxmid"] for r in sd), min(r["over"][2] for r in first), max(r["over"][2] for r in first), min(r["over"][2] for r in sd), max(r["over"][2] for r in sd)))
        # ---- d. zero vectors: optical flow's own work, dispatch by dispatch
        zv = [r for r in m if r["arm"] == "ZV"]
        if zv:
            curve = [(k, mean(r["mid"] for r in zv if r["k"] == k)) for k in range(1, 17) if any(r["k"] == k for r in zv)]
            mixc = [(k, mean(r["mix"] for r in zv if r["k"] == k)) for k in range(1, 17) if any(r["k"] == k for r in zv)]
            sd = mean(r["mid"] for r in zv if r["k"] >= STEADY); one = dict(curve).get(1)
            falls = one is not None and sd is not None and sd <= one - 0.01
            back = next((k for k, v in curve if falls and abs(v - sd) <= 0.1 * abs(one - sd)), None)
            answer(sid, "1d. arm ZV, every vector zero: from the half-way picture by dispatch: %s. From the EVEN MIX of the two frames: %s. Dispatch 1 reads %s from the half-way picture, the steady pairs %s: %s"
                   % (", ".join("%d: %.4f" % kv for kv in curve), ", ".join("%d: %.4f" % kv for kv in mixc), f3(one), f3(sd),
                      "optical flow's part is back from dispatch %d on (within a tenth of the way)" % back if back else "the distance does NOT fall towards the half-way picture: on this picture optical flow does not put right what the vectors leave out"))
        zl = [r for r in m if r["arm"] == "ZL"]
        if zl:
            ks = [k for k in (1, 9, 10, 12, 16, 20, 24, 28, 32, 36, 40, 44, 48) if any(r["k"] == k for r in zl)]
            tail = [mean(r["mix"] for r in zl if r["k"] == k) for k in range(37, 49) if any(r["k"] == k for r in zl)]
            mid = [mean(r["mix"] for r in zl if r["k"] == k) for k in range(25, 37) if any(r["k"] == k for r in zl)]
            settled = bool(tail) and bool(mid) and max(tail) - min(tail) <= 0.01 and abs(mean(tail) - mean(mid)) <= 0.01
            answer(sid, "1d. arm ZL, where that curve ends: from the even mix by dispatch: %s; from the half-way picture: %s. Dispatches 25 to 36 read %s from the mix, 37 to 48 read %s: %s"
                   % (", ".join("%d: %.4f" % (k, mean(r["mix"] for r in zl if r["k"] == k)) for k in ks), ", ".join("%d: %.4f" % (k, mean(r["mid"] for r in zl if r["k"] == k)) for k in ks),
                      span(mid), span(tail), "SETTLED by dispatch 37 (the last twelve within 0.01 of each other and of the twelve before)" if settled else "NOT SETTLED at dispatch 48"))


# ---------------------------------------------------------------------------------------------------------------- item 2
def whole_words(rows):
    q = [mean(r["q%d" % i] for r in rows) for i in range(4)]
    blk = [mean(r["blk"][i] for r in rows) for i in range(144)]
    return q, blk


def item2(S):
    res = {}
    for sid, gname in (("G2D", "DLAA guides (3840x2160)"), ("G2Q", "Quality guides (2560x1440)"), ("G2H", "DLAA guides (3840x2160), THE PICTURE AT 14 TIMES ITS VALUES (0.3 to 12.3: highlights in motion; \"over\" counts are at 14 times their lines too)")):
        s = S.get(sid)
        if not s: continue
        print("\nITEM 2, THE WHOLE IMAGE BY PLACE, %s under a 3840x2160 display (session %s; %s)" % (gname, sid, load_words(s["machine"])))
        if not instrument(s): continue
        m = [r for r in moves(s) if r["item"] == "2"]
        bad = [r for r in m if r["k"] < STEADY]
        proof(m and not bad, sid, "every reading is of a steady pair: the earliest dispatch read was the %dth after a reset (%d or more)" % (min(r["k"] for r in m) if m else -1, STEADY))
        for arm, what in (("F", "the right vectors, the peak on 'G' 1000"), ("F80", "the same pairs, the peak on 'G' 80"), ("F0", "the same pairs, the peak on 'G' 0 (the helper reads 1000)"), ("FN", "every vector negated (the control)")):
            rows = [r for r in m if r["arm"] == arm]
            if not rows: continue
            q, blk = whole_words(rows)
            print("  arm %s, %s, %d steady pairs, the whole image: %s from the half-way picture, %s from this frame's" % (arm, what, len(rows), span([r["mid"] for r in rows]), span([r["cur"] for r in rows])))
            print("    its inside (64 pixels in from every border) %s; the four borders, 64 pixels deep: top %s, bottom %s, left %s, right %s" % (
                span([r["imid"] for r in rows]), f3(mean(r["bt"] for r in rows)), f3(mean(r["bb"] for r in rows)), f3(mean(r["bl"] for r in rows)), f3(mean(r["br"] for r in rows))))
            print("    the four quadrants: upper left %s, upper right %s, lower left %s, lower right %s; the largest single pixel %.4f; pixels over 0.01: %d to %d, over 0.05: %d to %d, over 0.25: %d to %d" % (
                f3(q[0]), f3(q[1]), f3(q[2]), f3(q[3]), max(r["maxmid"] for r in rows), min(r["over"][1] for r in rows), max(r["over"][1] for r in rows),
                min(r["over"][2] for r in rows), max(r["over"][2] for r in rows), min(r["over"][3] for r in rows), max(r["over"][3] for r in rows)))
            print("    of the pixels over 0.05: within 8 pixels of a border %d to %d a pair, 64 pixels and more from every border %d to %d a pair" % (
                min(r["ring"] for r in rows), max(r["ring"] for r in rows), min(r["far"] for r in rows), max(r["far"] for r in rows)))
            print("    alpha: %s" % alpha_words(rows))
            print("    the 16 by 9 grid (each block 240x240, its mean over the pairs):")
            for y in range(9): print("      " + " ".join("%.4f" % blk[y * 16 + x] for x in range(16)))
            ib = inner_blocks(blk)
            res[(sid, arm)] = {"imid": mean(r["imid"] for r in rows), "q": q, "blk": blk, "inner_worst": max(ib), "mid": mean(r["mid"] for r in rows),
                               "bands": [mean(r[b] for r in rows) for b in ("bt", "bb", "bl", "br")], "a1": (min(r["a1"] for r in rows), max(r["a1"] for r in rows))}
        # ---- the peak on 'G': the same pairs at three peaks, by the whole image's hash
        hs = {}
        for t, r in s["rows"]:
            if t == "desk-hash": hs.setdefault(r["arm"], {})[int(r["i"])] = (r["col"], r["all"])
        if "F" in hs and "F80" in hs and "F0" in hs:
            n = len(hs["F"]); e80 = sum(1 for i in hs["F"] if hs["F80"].get(i) == hs["F"][i]); e0 = sum(1 for i in hs["F"] if hs["F0"].get(i) == hs["F"][i])
            proof(n > 0 and e0 == n, sid, "THE SAME PAIRS GIVE THE SAME IMAGE TWICE: arm F0 (the peak 0, which the helper reads as 1000) is arm F bit for bit, alpha included, on %d of %d pairs" % (e0, n))
            pd = [r for t, r in s["rows"] if t == "desk-peak-diff"]
            for arm in ("F80", "F0"):
                ra = [r for r in pd if r["arm"] == arm]
                if not ra: continue
                dif = [r for r in ra if int(r["differ"]) > 0]
                print("  arm %s against arm F, pixel by pixel, on %d pairs (every second one): %d pairs differ%s" % (arm, len(ra), len(dif), "" if not dif else
                      ": pixels that differ %d to %d a pair (within 8 pixels of a border %d to %d, 64 and more inside %d to %d); the largest difference %.6g (%d code steps, %.3g of the pixel's value), the mean of the differences %.3g; box of the widest %s"
                      % (min(int(r["differ"]) for r in dif), max(int(r["differ"]) for r in dif), min(int(r["edge"]) for r in dif), max(int(r["edge"]) for r in dif), min(int(r["inside"]) for r in dif), max(int(r["inside"]) for r in dif),
                         max(float(r["max"]) for r in dif), max(int(r["maxsteps"]) for r in dif), max(float(r["maxrel"]) for r in dif), mean(float(r["mean"]) for r in dif), max(dif, key=lambda r: int(r["differ"]))["box"])))
            d80 = [r for r in pd if r["arm"] == "F80" and int(r["differ"]) > 0]
            answer(sid, "2/3d. THE PEAK ON 'G' %s (%s): with the peak at 80 the whole generated image is bit for bit the image at 1000 on %d of %d steady pairs%s%s"
                   % ("DOES NOT REACH THE PICTURE" if e80 == n else "REACHES THE PICTURE", gname.split(" under")[0].split(", THE")[0] + (", the picture to 12.3" if sid == "G2H" else ", the picture to 0.88"), e80, n,
                      "" if e80 == n or (sid, "F80") not in res else "; arm F80 reads %.4f from the half-way picture against arm F's %.4f" % (res[(sid, "F80")]["mid"], res[(sid, "F")]["mid"]),
                      "" if not d80 else "; pixel by pixel on the pairs kept: %d to %d pixels differ a pair, %d to %d of them 64 pixels and more inside, the largest by %.6g (%d code steps, %.3g of the pixel's value)"
                      % (min(int(r["differ"]) for r in d80), max(int(r["differ"]) for r in d80), min(int(r["inside"]) for r in d80), max(int(r["inside"]) for r in d80),
                         max(float(r["max"]) for r in d80), max(int(r["maxsteps"]) for r in d80), max(float(r["maxrel"]) for r in d80))))

    def whole(x, ref):
        return x["imid"] <= 0.02 and (ref is None or x["imid"] <= ref["imid"] + 0.01) and max(x["q"]) <= max(2 * min(x["q"]), min(x["q"]) + 0.005) and x["inner_worst"] <= 0.05
    d, q = res.get(("G2D", "F")), res.get(("G2Q", "F"))
    if d:
        answer("G2D", "2. AT DLAA GUIDES THE IMAGE IS %s: its inside %.4f from the half-way picture, the quadrants %s, the worst block that touches no border %.4f, the borders (top, bottom, left, right) %s"
               % ("WHOLE" if whole(d, None) else "NOT WHOLE", d["imid"], " ".join("%.4f" % v for v in d["q"]), d["inner_worst"], " ".join("%.4f" % v for v in d["bands"])))
    if q:
        answer("G2Q", "2. AT QUALITY GUIDES (2560x1440 under 3840x2160) THE IMAGE IS %s: its inside %.4f from the half-way picture%s, the quadrants %s, the worst block that touches no border %.4f, the borders (top, bottom, left, right) %s"
               % ("WHOLE" if whole(q, d) else "NOT WHOLE", q["imid"], (" against %.4f at DLAA guides" % d["imid"]) if d else "", " ".join("%.4f" % v for v in q["q"]), q["inner_worst"], " ".join("%.4f" % v for v in q["bands"])))
    h = res.get(("G2H", "F"))
    if h:
        answer("G2H", "3d. HIGHLIGHTS IN MOTION (the picture at 14 times its values, 0.3 to 12.3) ARE %s: the inside %.4f from the half-way picture%s, the quadrants %s, the worst block that touches no border %.4f, the borders %s"
               % ("WHOLE" if whole(h, d) else "NOT WHOLE", h["imid"], (" against %.4f at the picture's own values" % d["imid"]) if d else "", " ".join("%.4f" % v for v in h["q"]), h["inner_worst"], " ".join("%.4f" % v for v in h["bands"])))
    for sid in ("G2D", "G2Q", "G2H"):
        x = res.get((sid, "FN"))
        if x: proof(not whole(x, None), sid, "THE CONTROL: the same judgement FAILS on arm FN (every vector negated): its inside is %.4f from the half-way picture, the worst block that touches no border %.4f" % (x["imid"], x["inner_worst"]))
        elif S.get(sid): proof(False, sid, "THE CONTROL: arm FN left no readings")


# ---------------------------------------------------------------------------------------------------------------- item 3
def item3(S):
    s = S.get("H3")
    if not s: return
    print("\nITEM 3, HDR BY BITS: still pairs, the whole generated image against what went in (session H3; %s)" % load_words(s["machine"]))
    if not instrument(s): return
    rows = [r for t, r in s["rows"] if t == "desk-const"]
    for r in rows:
        for k in ("px", "d0", "d1", "d2", "k", "blocks", "akinds", "amore", "maxsteps", "beyond"): r[k] = int(r[k])
        for k in ("max0", "max1", "max2", "mean0", "mean1", "mean2", "maxrel", "relpix"): r[k] = float(r[k])
    print("  each case is fourteen still pairs, the first told a cut; GEN is read behind dispatch 0 (the reset), 1, 2, 6, 10 and 13. \"differ\" is pixels of %d whose three colours are not bit for bit what went into PRESENT and HUDLESS." % PIXELS)
    print("  fp16 channels are red, green, blue, alpha; the 8-bit image is B8G8R8A8, so its first channel is blue. Alpha is listed by value (hex) and count.")
    cases = []
    for r in rows:
        key = (r["fmt"], r["case"], r["peak"])
        if key not in cases: cases.append(key)
    worst = {}
    alpha_seen = {}
    for fmt, case, peak in cases:
        rk = [r for r in rows if (r["fmt"], r["case"], r["peak"]) == (fmt, case, peak) and r["ctl"] == "none"]
        if not rk: continue
        last = rk[-1]
        al = "/".join(p for p in last["alpha"].split("/") if not p.endswith(":0"))
        big = max(rk, key=lambda r: r["px"])
        print("  %-5s %-13s peak %-5s differ by dispatch: %s; the largest difference: %d code step(s) (%d pixels more than one step off), by channel %.6g %.6g %.6g, mean by channel %.3g %.3g %.3g, at most %.3g of the pixel's own largest channel; in %d of 144 blocks, box %s; first pixel got %s, sent %s; alpha %s"
              % (fmt, case, peak, ", ".join("%d: %d" % (r["k"], r["px"]) for r in rk), max(r["maxsteps"] for r in rk), max(r["beyond"] for r in rk), max(r["max0"] for r in rk), max(r["max1"] for r in rk), max(r["max2"] for r in rk),
                 big["mean0"], big["mean1"], big["mean2"], max(r["relpix"] for r in rk), max(r["blocks"] for r in rk), big["box"], last["got"], last["want"], al))
        if 0 < big["blocks"] < 144:
            print("        where (the 16 by 9 grid, 1 a block with a pixel that differs): " + " ".join(big["blkmap"][y * 16:(y + 1) * 16] for y in range(9)))
        worst[(fmt, case, peak)] = (max(r["px"] for r in rk), max(max(r["max0"], r["max1"], r["max2"]) for r in rk), max(r["relpix"] for r in rk), [r["px"] for r in rk], max(r["maxsteps"] for r in rk), max(r["beyond"] for r in rk))
        alpha_seen[(fmt, case, peak)] = (al, rk[0]["alpha"])
    const = {k: v for k, v in worst.items() if k[0] == "fp16" and k[1] not in ("picture",) and not k[1].startswith("alpha")}
    exact = [k for k, v in const.items() if v[0] == 0]
    answer("H3", "3a. FIVE CONSTANTS AT FOUR PEAKS, FP16 scRGB: %d of %d cases come back BIT FOR BIT on every pixel of every reading (the reset dispatch and dispatches 1, 2, 6, 10, 13)%s"
           % (len(exact), len(const), "" if len(exact) == len(const) else ". NOT EXACT: " + "; ".join("%s at peak %s: up to %d pixels, the largest difference %d code step(s), %.6g, %.3g of the pixel's largest channel" % (k[1], k[2], v[0], v[4], v[1], v[2]) for k, v in const.items() if v[0])))
    pic = {k: v for k, v in worst.items() if k[1] == "picture"}
    if pic:
        answer("H3", "3b. A STILL PICTURE (red 0.3 to 12.3, green 4 times the waves, blue negative over half of it): %s"
               % "; ".join("peak %s: %s" % (k[2], "BIT FOR BIT on every reading" if v[0] == 0 else "differs on %s pixels by dispatch, the largest difference %.6g, at most %.3g of the pixel's own largest channel; %d pixels have a channel more than one code step off (the largest: %d steps)" % (v[3], v[1], v[2], v[5], v[4])) for k, v in pic.items()))
    al = [(k, v) for k, v in alpha_seen.items() if k[1].startswith("alpha") or (k[0] == "fp16" and k[1] == "1" and k[2] == "1000") or (k[0] == "bgra8" and k[1] == "white")]
    if al:
        answer("H3", "3c. ALPHA in the generated image (value in hex: pixels), steady and behind the reset dispatch: %s. The seed was BC00 in fp16 (-1.0) and 63 in the 8-bit image; 1.0 is 3C00 and FF"
               % "; ".join("%s %s (PRESENT and HUDLESS as named): %s steady, %s at the reset" % (k[0], k[1], v[0], "/".join(p for p in v[1].split("/") if not p.endswith(":0"))) for k, v in al))
    b8 = {k: v for k, v in worst.items() if k[0] == "bgra8"}
    if b8:
        answer("H3", "3, the 8-bit set (sRGB): %d of %d cases come back BIT FOR BIT%s" % (sum(1 for v in b8.values() if v[0] == 0), len(b8),
               "" if all(v[0] == 0 for v in b8.values()) else ". NOT EXACT: " + "; ".join("%s: up to %d pixels, the largest difference %.4g of 1.0" % (k[1], v[0], v[1]) for k, v in b8.items() if v[0])))
    seed = [r for r in rows if r["ctl"] == "seed"]; seedis = [r for r in rows if r["ctl"] == "seedis"]; step = [r for r in rows if r["ctl"] == "step"]
    proof(seed and seedis and step and all(r["px"] == PIXELS for r in seed) and all(r["px"] == 0 for r in seedis) and all(r["d0"] == PIXELS for r in step), "H3",
          "THE CONTROLS: in %d cases the seed read back before 'G' differs from what was sent on every pixel (the least: %d of %d) and IS the seed (the most pixels that are not: %d); and in %d cases the answer compared with a value ONE CODE STEP off in its first channel differs on every pixel (the least: %d)"
          % (len(seed), min(r["px"] for r in seed) if seed else 0, PIXELS, max(r["px"] for r in seedis) if seedis else -1, len(step), min(r["d0"] for r in step) if step else 0))


# ---------------------------------------------------------------------------------------------------------------- item 4
PLANTS = ("one code step", "+0.004", "+0.02", "+0.05", "+0.10", "+0.25", "nothing planted")


def item4(S):
    for sid, what in (("U4", "HUDLESS as the game would send it"), ("U4E", "HUDLESS EMPTY (the helper's --fghud_empty=1; the rows say OWN, SAME and STALE for what this side wrote, and the helper read none of it)")):
        s = S.get(sid)
        if not s: continue
        print("\nITEM 4, THE HUD'S RECTANGLE, %s (session %s; %s)" % (what, sid, load_words(s["machine"])))
        if not instrument(s): continue
        if sid == "U4E": proof("EMPTY HUD-less colour" in s["log"], sid, "the helper's log says every configure handed AMD an EMPTY HUD-less colour")
        rows = [r for t, r in s["rows"] if t == "desk-hud"]
        print("  PRESENT carries a 512x256 rectangle (a colour that changes every pair) and six 128x128 plants (the picture plus an amount); steady pairs, twelve readings an arm.")
        print("  \"inside\" is the rectangle 32 pixels in from its edge (%s pixels). w is how far GEN goes from the half-way picture towards what PRESENT held: 0 the half-way picture, 1 the presented pixel."
              % (rows[0]["an"] if rows else "?"))
        first = {r["arm"]: r for r in rows if int(r["i"]) == 0}
        rows = [r for r in rows if int(r["i"]) >= 14]
        for arm, aw in (("OWN", "HUDLESS is the picture without the rectangle and the plants"), ("SAME", "HUDLESS is PRESENT, rectangle and plants and all"), ("STALE", "HUDLESS is the pair before's picture")):
            ra = [r for r in rows if r["arm"] == arm]
            if not ra: continue
            an = int(ra[0]["an"]); this = [int(r["athis"]) for r in ra]; prev = [int(r["aprev"]) for r in ra]; pn = int(ra[0]["pn"])
            print("  arm %s (%s):" % (arm, aw))
            print("    inside the rectangle: this pair's colour exactly on %d to %d of %d pixels, the pair before's on %d to %d; mean difference from this pair's colour %.6f"
                  % (min(this), max(this), an, min(prev), max(prev), mean(float(r["aerr"]) for r in ra)))
            print("    far from the rectangles: %s from the half-way picture, %s from this frame's" % (span([float(r["far_mid"]) for r in ra]), span([float(r["far_cur"]) for r in ra])))
            for p in range(7):
                print("    plant %d (%-15s, the picture there %.3f on average): w %s; from the half-way picture %s, from the presented pixel %s; bit for bit the presented pixel on %d to %d of %d, the unplanted picture on %d to %d"
                      % (p, PLANTS[p], mean(float(r["pv%d" % p]) for r in ra), span([float(r["w%d" % p]) for r in ra]), f3(mean(float(r["pm%d" % p]) for r in ra)), f3(mean(float(r["pc%d" % p]) for r in ra)),
                         min(int(r["eqp%d" % p]) for r in ra), max(int(r["eqp%d" % p]) for r in ra), pn, min(int(r["eqc%d" % p]) for r in ra), max(int(r["eqc%d" % p]) for r in ra)))
            for sd, sname in enumerate(("left", "right", "top", "bottom")):
                prof = [mean(float(r["edge%d" % sd].split(";")[d]) for r in ra) for d in range(64)]
                off = [d - 32 for d in range(64) if prof[d] > 0.02]
                print("    the %s edge, by pixels INTO the rectangle (-32 outside to 31): the difference from what should be there, as a part of the contrast: over 0.02 %s; largest %.4f"
                      % (sname, ("from %d to %d (%d pixels)" % (min(off), max(off), len(off))) if off else "nowhere", max(prof)))
                print("      " + " ".join("%.3f" % v for v in prof))
        own = [r for r in rows if r["arm"] == "OWN"]; same = [r for r in rows if r["arm"] == "SAME"]; stale = [r for r in rows if r["arm"] == "STALE"]
        for arm in ("OWN", "SAME", "STALE"):
            f = first.get(arm)
            if f: print("  arm %s, ITS RESET DISPATCH (the arm's first pair): inside the rectangle this pair's colour on %s of %s pixels; far from the rectangles %s from this frame's picture; the plants, bit for bit the presented pixel / the unplanted picture, of %s: %s"
                        % (arm, f["athis"], f["an"], f["far_cur"], f["pn"], "; ".join("%s %s/%s" % (PLANTS[p], f["eqp%d" % p], f["eqc%d" % p]) for p in range(7))))
        if first.get("OWN"):
            f = first["OWN"]
            hud = f["athis"] == f["an"] and f["eqp5"] == f["pn"]; step_present = f["eqp0"] == f["pn"]; step_hudless = f["eqc0"] == f["pn"]
            answer(sid, "4, WHICH INPUT A RESET DISPATCH COPIES (%s): in arm OWN its inside of the rectangle is this pair's colour on %s of %s pixels, the +0.25 plant is the presented pixel on %s of %s, and the one-step plant is the presented pixel on %s and the HUD-less picture on %s of %s: %s"
                   % ("HUDLESS as the game would send it" if sid == "U4" else "HUDLESS empty", f["athis"], f["an"], f["eqp5"], f["pn"], f["eqp0"], f["eqc0"], f["pn"],
                      "PRESENT, bit for bit, plants and all" if hud and step_present else
                      "THE HUD-LESS PICTURE WITH PRESENT PUT BACK WHERE THE TWO DIFFER (the rectangle and the large plants are PRESENT's, a one-step difference stays the HUD-less picture's)" if hud and step_hudless else
                      "neither input whole"))
        if sid == "U4" and own:
            full = all(int(r["athis"]) == int(r["an"]) for r in own)
            answer(sid, "4a. INSIDE THE RECTANGLE THE GENERATED IMAGE IS %s: this pair's colour exactly on %d to %d of %s pixels over %d readings, the pair before's on %d to %d"
                   % ("THIS PAIR'S COLOUR, EXACTLY, ON EVERY PIXEL" if full else "NOT this pair's colour on every pixel", min(int(r["athis"]) for r in own), max(int(r["athis"]) for r in own), own[0]["an"], len(own),
                      min(int(r["aprev"]) for r in own), max(int(r["aprev"]) for r in own)))
            w = [mean(float(r["w%d" % p]) for r in own) for p in range(7)]
            answer(sid, "4c. THE PLANTS (w: 0 the half-way picture, 1 the presented pixel): %s. ONE CODE STEP is %s (w %.4f against %.4f where nothing was planted)"
                   % ("; ".join("%s %.4f" % (PLANTS[p], w[p]) for p in range(7)), "NOT taken for HUD" if abs(w[0] - w[6]) <= 0.02 else "TAKEN FOR HUD", w[0], w[6]))
        if sid == "U4" and stale:
            answer(sid, "4d. arm STALE (HUDLESS is the pair before's picture): far from the rectangles GEN is %s from this frame's picture and %s from the half-way picture"
                   % (span([float(r["far_cur"]) for r in stale]), span([float(r["far_mid"]) for r in stale])))
        if sid == "U4" and same:
            bites = any(int(r["athis"]) != int(r["an"]) for r in same)
            proof(bites, sid, "THE CONTROL: \"inside the rectangle every pixel of every reading is this pair's colour\" FAILS on arm SAME (the rectangle in both inputs): this pair's colour on %d to %d of %s pixels"
                  % (min(int(r["athis"]) for r in same), max(int(r["athis"]) for r in same), same[0]["an"]))
        if sid == "U4E" and own:
            answer(sid, "4e. HUDLESS EMPTY: inside the rectangle this pair's colour exactly on %d to %d of %s pixels, the pair before's on %d to %d; mean difference from this pair's colour %.4f; the one-step plant's w %.4f"
                   % (min(int(r["athis"]) for r in own), max(int(r["athis"]) for r in own), own[0]["an"], min(int(r["aprev"]) for r in own), max(int(r["aprev"]) for r in own),
                      mean(float(r["aerr"]) for r in own), mean(float(r["w0"]) for r in own)))


# ---------------------------------------------------------------------------------------------------------------- item 5
def item5(S):
    s = S.get("A5")
    if not s: return
    print("\nITEM 5, THE HELD ANSWER, BY PIXELS (session A5; %s)" % load_words(s["machine"]))
    if not instrument(s): return
    held = [r for t, r in s["rows"] if t == "desk-held"]
    names = {"answer": "at its answer (fgOut 2n)", "later": "1.5 s later", "nextframe": "behind the next frame's message and its answer", "nextQ": "behind the next 'Q' and fgPrep (AMD's configure and prepare for n+1)",
             "nextcopies": "behind the next pair's copies into PRESENT and HUDLESS, fgIn withheld", "nextG": "behind the next 'G' (DISPATCH), 1.5 s with fgIn withheld", "nextIn": "behind fgIn for the next pair and its answer"}
    for r in held:
        print("  pair n's image %-78s fgOut reads %-4s the whole image's hash %s: %s (%s of 144 blocks the same)"
              % (names.get(r["step"], r["step"]) + ":", r["fgout"], r["hash"], "THE SAME, bit for bit, alpha included" if r["same"] == "1" else "NOT THE SAME", r["blocks"]))
    mv = {r["arm"]: r for r in moves(s) if r["item"] == "5"}
    if "answer" in mv and "next" in mv:
        print("  what the two images are: pair n's is %.4f from its own half-way picture; the image behind fgIn is %.4f from pair n+1's half-way picture" % (mv["answer"]["mid"], mv["next"]["mid"]))
    still = [r for r in held if r["step"] in ("later", "nextframe", "nextQ", "nextcopies", "nextG")]
    ok = len(still) == 5 and all(r["same"] == "1" for r in still)
    answer("A5", "5. THE GENERATED IMAGE OF PAIR n %s: %d of 5 readings between its answer and the game's own signal for the next pair are the same image bit for bit%s"
           % ("STANDS STILL UNTIL THE GAME'S OWN SIGNAL FOR THE NEXT PAIR" if ok else "DOES NOT STAND STILL", sum(1 for r in still if r["same"] == "1"),
              "" if ok else "; it changed at: " + ", ".join(r["step"] for r in still if r["same"] != "1")))
    nxt = [r for r in held if r["step"] == "nextIn"]
    order = [r for t, r in s["rows"] if t == "desk-order"]
    w = {k: [r for r in order if r["way"] == str(k)] for k in (0, 1, 2)}
    for k, what in ((0, "the read submitted BEFORE the submit that signals fgIn, nothing waited (the rule)"), (1, "the read submitted right BEHIND that submit, nothing waited (the rule broken)"),
                    (2, "the read submitted after the next pair's answer (the control)")):
        r = w[k]
        if not r: continue
        torn = [x for x in r if x["is_p"] != "1" and x["is_q"] != "1"]
        print("  the queue's order, %s: %d trials: pair p's image whole in %d, pair q's whole in %d, neither in %d%s" % (what, len(r), sum(1 for x in r if x["is_p"] == "1"), sum(1 for x in r if x["is_q"] == "1"), len(torn),
              ("; of 144 blocks p's / q's: " + ", ".join("%s/%s" % (x["blocks_p"], x["blocks_q"]) for x in torn[:8])) if torn else ""))
    if w[0]:
        answer("A5", "5, THE RULE ON THE QUEUE: a read of GEN submitted before the submit that signals fgIn held pair p's image whole in %d of %d trials; a read submitted right behind it held p's in %d, q's in %d and neither in %d of %d"
               % (sum(1 for x in w[0] if x["is_p"] == "1"), len(w[0]), sum(1 for x in w[1] if x["is_p"] == "1"), sum(1 for x in w[1] if x["is_q"] == "1"),
                  sum(1 for x in w[1] if x["is_p"] != "1" and x["is_q"] != "1"), len(w[1])))
    proof(len(nxt) == 1 and nxt[0]["same"] == "0" and w[2] and all(x["is_p"] == "0" and x["is_q"] == "1" for x in w[2]), "A5",
          "THE CONTROL: behind fgIn the same comparison FAILS (%s of 144 blocks the same), and a read submitted after the next pair's answer is never pair p's image and always pair q's (%d of %d trials)"
          % (nxt[0]["blocks"] if nxt else "?", sum(1 for x in w[2] if x["is_q"] == "1" and x["is_p"] == "0"), len(w[2])))


# ---------------------------------------------------------------------------------------------------------------- item 6
def item6(S):
    s = S.get("Z6")
    if not s: return
    print("\nITEM 6, TWO DEPTH PLANES: a 960x640 object moves over a still plane (session Z6; %s)" % load_words(s["machine"]))
    if not instrument(s): return
    rows = [r for t, r in s["rows"] if t == "desk-depth"]; diffs = {r["arm"]: r for t, r in s["rows"] if t == "desk-depth-diff"}
    print("  the depth is the game's (near over Z: 1 at the near plane, towards 0 far away). Twelve steady readings an arm. Each figure is GEN's difference from what should be there")
    print("  (the object at its half-way place over the plane) as a part of the contrast between the object's picture and the plane's at that pixel; the band is 48 pixels each side of the object's edge.")
    print("  arm  object  plane  near on 'Q'   object's inside   band inside: left right top bottom       band outside: left right top bottom      plane near (absolute)   plane far (absolute)   far plane bit for bit")
    by = {}
    for arm in ("R", "S", "F0", "F1", "N1", "N2", "V", "R2"):
        ra = [r for r in rows if r["arm"] == arm]
        if not ra: continue
        g = lambda k: mean(float(r[k]) for r in ra)
        by[arm] = {k: g(k) for k in ("obj", "in_l", "in_r", "in_t", "in_b", "out_l", "out_r", "out_t", "out_b", "near_abs", "far_abs")}
        by[arm]["far_max"] = max(float(r["far_max"]) for r in ra)
        by[arm]["hash"] = [r["col"] for r in ra]
        print("  %-4s %-7s %-6s %-13s %-17s %-40s %-40s %-23s %-36s %d to %d of %s" % (arm, ra[0]["zobj"], ra[0]["zbg"], ra[0]["near"], f3(g("obj")), " ".join(f3(g(k)) for k in ("in_l", "in_r", "in_t", "in_b")),
              " ".join(f3(g(k)) for k in ("out_l", "out_r", "out_t", "out_b")), "%.7f" % g("near_abs"), "%.7f (largest %.6f)" % (g("far_abs"), by[arm]["far_max"]), min(int(r["far_eq"]) for r in ra), max(int(r["far_eq"]) for r in ra), ra[0]["far_n"]))
    print("  against the first arm's image of the same pair (the 21st), pixel by pixel:")
    for arm in ("S", "F0", "F1", "N1", "N2", "V", "R2"):
        d = diffs.get(arm)
        if not d: continue
        same12 = sum(1 for a, b in zip(by.get(arm, {}).get("hash", []), by.get("R", {}).get("hash", [])) if a == b)
        print("    %-3s %s pixels differ (in the object's inside %s, in the band %s, in the plane %s), the largest by %s, box %s; the whole image's hash is the first arm's on %d of 12 readings"
              % (arm, d["differ"], d["in_object"], d["in_band"], d["in_plane"], d["max"], d["box"], same12))
    r2 = diffs.get("R2"); v = diffs.get("V")
    proof(r2 is not None and int(r2["differ"]) == 0 and by.get("R2", {}).get("hash") == by.get("R", {}).get("hash"), "Z6",
          "THE FIRST ARM RUN AGAIN GIVES THE FIRST ARM'S IMAGE: %s pixels differ on the 21st pair, and the whole image's hash is the same on every one of the twelve readings" % (r2["differ"] if r2 else "?"))
    proof(v is not None and int(v["differ"]) > 0, "Z6", "THE CONTROL: \"the same image as the first arm\" FAILS on arm V (the object's vectors left out): %s pixels differ, the largest by %s" % (v["differ"] if v else "?", v["max"] if v else "?"))
    if "R" in by and "S" in by and diffs.get("S"):
        ds = diffs["S"]; band = lambda a: mean(by[a][k] for k in ("in_l", "in_r", "in_t", "in_b", "out_l", "out_r", "out_t", "out_b"))
        answer("Z6", "6. THE PLANES SWAPPED %s: %s pixels differ from the right planes' image on the 21st pair (object's inside %s, band %s, plane %s), the largest by %s. At the object's edge (the band, both sides, four edges) the right planes read %.4f, swapped %.4f, one plane at zero %.4f, one plane at 0.1 %.4f; the object's inside %.4f, %.4f, %.4f, %.4f"
               % ("SHOW IN THE PICTURE" if int(ds["differ"]) > 0 else "DO NOT SHOW IN THE PICTURE: the image is the same", ds["differ"], ds["in_object"], ds["in_band"], ds["in_plane"], ds["max"],
                  band("R"), band("S"), band("F0") if "F0" in by else -1, band("F1") if "F1" in by else -1, by["R"]["obj"], by["S"]["obj"], by.get("F0", {}).get("obj", -1), by.get("F1", {}).get("obj", -1)))
    if diffs.get("N1") and diffs.get("N2"):
        answer("Z6", "6, THE NEAR PLANE ON 'Q' (the helper passes it as cameraFar, with cameraNear at the largest float): at 1.0 against 0.05, %s pixels differ (the largest by %s); at 100 against 0.05, %s pixels differ (the largest by %s)"
               % (diffs["N1"]["differ"], diffs["N1"]["max"], diffs["N2"]["differ"], diffs["N2"]["max"]))


# ---------------------------------------------------------------------------------------------------------------- item 7
def item7(S):
    ids = ["X7"] + ["X7" + c for c in "abcdef"]
    if not any(S.get(i) for i in ids): return
    print("\nITEM 7, THE HELPER KILLED WITH A PAIR PENDING (LS-4): what the game's imports hold afterwards")
    held_ok = None; ctl = []
    for sid in ids:
        s = S.get(sid)
        if not s: continue
        print("  session %s (%s):" % (sid, load_words(s["machine"])))
        if not instrument(s): continue
        f = next((r for t, r in s["rows"] if t == "desk-dead-fence"), None)
        dead = [r for t, r in s["rows"] if t == "desk-dead"]; c = next((r for t, r in s["rows"] if t == "desk-dead-control"), None)
        end = next((r for t, r in s["rows"] if t == "desk-dead-end"), None)
        if not f: proof(False, sid, "the fences' row is missing"); continue
        print("    pair %s pending (its own value on the answer fence would be %s); killed %s us after 'G' was written, the answer fence then read %s. AFTER: fgPrep %s, fgOut %s, fgIn %s, the frame's in-fence %s, its out-fence %s"
              % (f["pending"], f["want"], f["killus"], f["out_before"], f["prep"], f["out"], f["fgin"], f["frame_in"], f["frame_out"]))
        print("    a wait on the answer fence for %s: %s after %s us. \"At least %s\": %s. \"Equal to %s\": %s. At the session's end the fences read %s and %s"
              % (f["want"], "RETURNED" if f["wait_returned"] == "1" else "did not return in 200 ms", f["wait_us"], f["want"], "TRUE" if f["at_least"] == "1" else "false", f["want"], "TRUE" if f["equal"] == "1" else "false",
                 end["prep"] if end else "?", end["out"] if end else "?"))
        g = lambda step, image: next((r for r in dead if r["step"] == step and r["image"] == image), None)
        for step, sw in (("dead", "at once"), ("dead3s", "3 s later")):
            print("    the images through the imports, %s: %s" % (sw, "; ".join("%s %s" % (im, "?" if not g(step, im) else ("the same bit for bit as %s" % ("before the kill" if step == "dead" or im != "GEN" else "at once")) if g(step, im)["same"] == "1"
                                                                                          else "NOT the same (%s of 144 blocks)" % g(step, im)["blocks"]) for im in ("PRESENT", "HUDLESS", "GEN"))))
        mv = {r["arm"]: r for r in moves(s) if r["item"] == "7"}
        if "gen_dead" in mv:
            x = mv["gen_dead"]
            print("    GEN after the kill against pair %s's two pictures: %.4f from the half-way picture, %.4f from this frame's, %.4f from the previous frame's (pair n's own image reads about 0.5 from this half-way picture; pair n+1's whole image about 0.002)"
                  % (f["pending"], x["mid"], x["cur"], x["prev"]))
        if "present_alive" in mv:
            proof(mv["present_alive"]["eqcur"] == PIXELS, sid, "the instrument: with the helper alive PRESENT read through the import is the picture this side wrote, bit for bit (%d of %d pixels)" % (mv["present_alive"]["eqcur"], PIXELS))
        pr = g("product", "BACKBUFFER")
        print("    the product's order on PRESENT (acquire, copy into a back buffer of this side's, release), the back buffer read: %s" % ("?" if not pr else "PRESENT's picture bit for bit" if pr["same"] == "1" else "NOT PRESENT's picture (%s of 144 blocks)" % pr["blocks"]))
        cr = g("control", "PRESENT")
        ctl.append((sid, cr is not None and cr["same"] == "0" and c is not None and int(c["written_differs"]) == 0, c["written_differs"] if c else "?", cr["blocks"] if cr else "?"))
        alive = all(g(st, im) is not None and g(st, im)["same"] == "1" for st in ("dead", "dead3s") for im in ("PRESENT", "HUDLESS")) and pr is not None and pr["same"] == "1"
        if sid == "X7":
            held_ok = alive
            answer(sid, "7. THE IMPORTS %s: with the helper killed (its dispatch for pair %s held behind fgIn), PRESENT and HUDLESS read %s at once and 3 s later, GEN %s, and the product's order on PRESENT gives %s"
                   % ("OUTLIVE THE HELPER" if alive else "DO NOT ALL OUTLIVE THE HELPER", f["pending"], "the same bit for bit" if alive else "NOT all the same",
                      "still holds pair n's image bit for bit" if g("dead", "GEN") and g("dead", "GEN")["same"] == "1" else "does NOT hold pair n's image", "PRESENT's picture" if pr and pr["same"] == "1" else "something else"))
            answer(sid, "7, THE ANSWER FENCE OF A DEAD HELPER reads %s with pair %s pending (its own value %s): a wait for that value %s, \"at least\" is %s and EQUALITY IS %s"
                   % (f["out"], f["pending"], f["want"], "returned after %s us" % f["wait_us"] if f["wait_returned"] == "1" else "did not return", "TRUE" if f["at_least"] == "1" else "false", "TRUE" if f["equal"] == "1" else "FALSE"))
        else:
            x = mv.get("gen_dead"); gd = g("dead", "GEN")
            what = "pair n's image, untouched" if gd and gd["same"] == "1" else ("pair n+1's image, whole" if x and x["mid"] <= 0.02 else "NEITHER pair's whole image")
            answer(sid, "7, killed %s us after 'G' with the dispatch in flight: fgOut read %s before the kill and %s after; GEN holds %s (%s of 144 blocks are pair n's; %s from pair n+1's half-way picture); PRESENT and HUDLESS %s"
                   % (f["killus"], f["out_before"], f["out"], what, gd["blocks"] if gd else "?", ("%.4f" % x["mid"]) if x else "?", "the same bit for bit" if alive else "NOT the same"))
    if ctl:
        proof(all(c[1] for c in ctl), "X7", "THE CONTROL: \"the same image\" FAILS on PRESENT after a constant was written into it through the import, in %d of %d sessions, and every pixel then reads the constant (pixels that do not: %s)"
              % (sum(1 for c in ctl if c[1]), len(ctl), ", ".join(str(c[2]) for c in ctl)))


# ---------------------------------------------------------------------------------------------------------------- item 8
def k_stats(s, key="g_us"):
    v = [float(r[key]) for t, r in s["rows"] if t == "desk-k"]
    if not v: return None
    n = len(v); third = n // 3
    return {"n": n, "min": min(v), "p25": pct(v, 0.25), "p50": pct(v, 0.5), "p90": pct(v, 0.9), "p99": pct(v, 0.99), "p999": pct(v, 0.999), "max": max(v), "mean": mean(v),
            "thirds": [pct(v[i * third:(i + 1) * third], 0.5) for i in range(3)], "over": {ms: sum(1 for x in v if x > ms * 1000.0) for ms in (2.5, 3.0, 4.0, 5.0, 8.0, 15.0, 20.0)}, "v": v}


def clock_words(s):
    took = s["machine"]["took"]; smi = s["smi"]
    if not smi: return "the GPU's clock was not read"
    mhz = sorted(r[1] for r in smi); busy = sorted(r[2] for r in smi)
    return "GPU clock %.0f MHz at the middle of the session (%.0f to %.0f over %d reads), the card %.0f%% busy at the middle; %.0f s" % (mhz[len(mhz) // 2], mhz[0], mhz[-1], len(mhz), busy[len(busy) // 2], took)


def item8(S):
    if not any(k.startswith("K8") for k in S): return
    print("\nITEM 8, K AT A DESK: 'G' written to the answer seen, per pair, the card this pair's alone. %d timed pairs a take behind %d that are nobody's; every pair steady (only the first of a session resets)." % (K_PAIRS, K_WARM))
    print("  A FLOOR: no game on either queue. Microseconds. \"the hand\" is the return of the submit that signals fgIn, \"'Q'\" the write of 'Q'.")
    for g, gname in (("D", "DLAA guides (3840x2160)"), ("Q", "Quality guides (2560x1440)")):
        for order, oname in ((0, "EVERY PART WAITED FOR before the next (K is the helper's read of 'G', AMD's recording, the dispatch on the card and the answer's way back)"),
                             (1, "THE GAME'S ORDER, nothing waited between a pair's parts (K also holds what is left of the frame, the prepare and the copies)")):
            takes = [(t, S.get("K8%s%d%s" % (g, order, t))) for t in "abc"]
            takes = [(t, s) for t, s in takes if s]
            if not takes: continue
            print("  %s, %s:" % (gname, oname))
            print("    take  pairs  least   25th    median  90th    99th    99.9th  largest  mean    over 2.5 / 3 / 4 / 5 / 8 ms   medians by thirds      polls a pair   from the hand: median 99th   from 'Q': median 99th")
            st = []; busy = []
            for t, s in takes:
                if not instrument(s): continue
                k = k_stats(s); ki = k_stats(s, "in_us"); kq = k_stats(s, "q_us")
                if not k: proof(False, s["sid"], "no timed pair was written"); continue
                polls = mean(float(r["polls"]) for tg, r in s["rows"] if tg == "desk-k")
                print("    %-5s %-6d %-7.0f %-7.0f %-7.0f %-7.0f %-7.0f %-7.0f %-8.0f %-7.0f %-29s %-22s %-14.0f %-28s %s" % (
                    t, k["n"], k["min"], k["p25"], k["p50"], k["p90"], k["p99"], k["p999"], k["max"], k["mean"], " / ".join(str(k["over"][ms]) for ms in (2.5, 3.0, 4.0, 5.0, 8.0)),
                    " ".join("%.0f" % x for x in k["thirds"]), polls, "%.0f %.0f" % (ki["p50"], ki["p99"]), "%.0f %.0f" % (kq["p50"], kq["p99"])))
                print("          the machine: %s; %s" % (load_words(s["machine"]), clock_words(s)))
                bym = {}
                for tg, r in s["rows"]:
                    if tg == "desk-k": bym.setdefault(int(r["move"]), []).append(float(r["g_us"]))
                print("          by the picture's move (AMD's dispatch costs a different time for each): least %s; lower quartiles %s; medians %s; 99th percentiles %s" % (
                    " ".join("%.0f" % min(bym[mvi]) for mvi in sorted(bym)), " ".join("%.0f" % pct(bym[mvi], 0.25) for mvi in sorted(bym)),
                    " ".join("%.0f" % pct(bym[mvi], 0.5) for mvi in sorted(bym)), " ".join("%.0f" % pct(bym[mvi], 0.99) for mvi in sorted(bym))))
                # the pairs that sit well above their own move's floor: a second mode, or a tail
                up = [x - min(bym[mvi]) for mvi in bym for x in bym[mvi] if x > min(bym[mvi]) + 350.0]
                lo = [x - min(bym[mvi]) for mvi in bym for x in bym[mvi] if x <= min(bym[mvi]) + 350.0]
                print("          against each move's own least: %d of %d pairs within 350 us of it (their median %.0f us above it), %d pairs (%.0f%%) more than 350 us above it (their median %.0f us above it, their largest %.0f)" % (
                    len(lo), k["n"], pct(lo, 0.5) if lo else 0.0, len(up), 100.0 * len(up) / k["n"], pct(up, 0.5) if up else 0.0, max(up) if up else 0.0))
                mv = [r for r in moves(s) if r["item"] == "8"]
                if mv: print("          the last pair's image, whole: %.4f from the half-way picture, %.4f from this frame's" % (mv[0]["mid"], mv[0]["cur"]))
                if s["machine"]["others"] or s["machine"]["game"]:
                    busy.append(t); print("          NOT IDLE: another GPU user ran during this take (%s): it is left out of the verdict below" % (s["machine"]["others"] or "the game"))
                else: st.append(k)
            if busy: print("    takes that were NOT IDLE and are left out: %s" % ", ".join(busy))
            if len(st) >= 2:
                p50 = [k["p50"] for k in st]; p99 = [k["p99"] for k in st]
                agree = max(p50) - min(p50) <= max(30.0, 0.02 * pct(p50, 0.5)) and max(p99) - min(p99) <= max(100.0, 0.10 * pct(p99, 0.5))
                answer("K8%s%d" % (g, order), "8. K AT A DESK, %s, %s: %d idle takes: least %s us, lower quartiles %s us, medians %s us, 99th percentiles %s us, 99.9th %s us, largest %s us; pairs over 3.0 ms: %s of %d each: %s"
                       % (gname.split(" (")[0], "every part waited for" if order == 0 else "the game's order", len(st), ", ".join("%.0f" % k["min"] for k in st), ", ".join("%.0f" % k["p25"] for k in st),
                          ", ".join("%.0f" % x for x in p50), ", ".join("%.0f" % x for x in p99),
                          ", ".join("%.0f" % k["p999"] for k in st), ", ".join("%.0f" % k["max"] for k in st), ", ".join(str(k["over"][3.0]) for k in st), st[0]["n"],
                          "THE TAKES AGREE (medians within 2 percent or 30 us, 99th percentiles within 10 percent or 100 us)" if agree else "THE TAKES DISAGREE: each stands alone and nothing is averaged"))
        s = S.get("K8%s0w" % g)
        if s and instrument(s):
            k = k_stats(s)
            if k: print("  %s, every part waited for, THE ANSWER SEEN BY A WAIT (vkWaitSemaphores) instead of by reading the fence: least %.0f, 25th %.0f, median %.0f, 90th %.0f, 99th %.0f, 99.9th %.0f, largest %.0f; %s; %s"
                        % (gname, k["min"], k["p25"], k["p50"], k["p90"], k["p99"], k["p999"], k["max"], load_words(s["machine"]), clock_words(s)))
    s = S.get("K8C")
    if s:
        print("  THE CONTROL (session K8C: the helper's --fgslow_d=%d, every 64th dispatch of the connection sleeps %d ms before it is recorded; %s):" % (SLOW_MS, SLOW_MS, load_words(s["machine"])))
        if instrument(s):
            k = k_stats(s)
            slow = [i for i, x in enumerate(k["v"]) if x >= SLOW_MS * 1000.0]
            want = [t - K_WARM - 1 for t in range(K_WARM + 1, K_WARM + K_PAIRS + 1) if t % 64 == 0]     # the lever counts the connection's dispatches from 1
            print("    least %.0f, median %.0f, 99th %.0f, largest %.0f; pairs over 15 ms: %d; at or over %d ms: %d, at timed pairs %s" % (k["min"], k["p50"], k["p99"], k["max"], k["over"][15.0], SLOW_MS, len(slow), slow[:6]))
            proof(k["over"][15.0] > 0 and slow == want, "K8C", "THE CONTROL: \"no pair over 15 ms\" FAILS (%d pairs), and the pairs at or over %d ms are exactly the %d the lever slept (every 64th dispatch of the connection: %d found, the same ones: %s)"
                  % (k["over"][15.0], SLOW_MS, len(want), len(slow), "yes" if slow == want else "NO"))
            idle = [S.get("K8D0%s" % t) for t in "abc"]
            worst = max((k_stats(x)["over"][15.0] for x in idle if x and k_stats(x)), default=None)
            if worst is not None: print("    the three idle takes at the same settings hold %d pairs over 15 ms at the most" % worst)


# ---------------------------------------------------------------------------------------------------------------- the run
def main():
    if not KEEP:
        if os.path.exists(OUT):
            left = end_ours("left by an earlier run")
            if left: time.sleep(1.0)
            shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT, exist_ok=True)
    sys.stdout = Tee(os.path.join(OUT, "result-read.txt" if READ else "result.txt"))
    print("THE DESK MEASUREMENTS, %s (frame generation phase 3, the build map's stage 1). The helper: %s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M"), HELPER))
    todo = plan()
    if not READ:
        d = prepare()
        if d is None:
            print("FAIL"); return 1
        for sid, item, title, hargs, fargs, marks in todo:
            session(d, sid, item, title, hargs, fargs, marks)
    S = {}
    for sid, item, title, hargs, fargs, marks in todo:
        s = read_session(sid)
        if s is None: proof(False, sid, "the session left nothing: it was not run"); continue
        S[sid] = s
    item1(S); item2(S); item3(S); item4(S); item5(S); item6(S); item7(S); item8(S)
    if shared: print("\nSESSIONS THAT RAN BESIDE ANOTHER GPU USER after 30 minutes of waiting: %s" % ", ".join(shared))
    if not READ:                                        # (a reading runs nothing and ends nothing: a run of this tool's may be going on beside it)
        left = end_ours("left at the end") if ours() else []
        proof(not left, "end", "no process of this tool's is left (%d had to be ended)" % len(left))
    print("\nTHE ANSWERS, one line each:")
    for sid, a in answers: print("  [%s] %s" % (sid, a))
    for sid, l in fails[:20]: print("  failed: [%s] %s" % (sid, l.strip()[:260]))
    print("%d proofs passed, %d failed; the folder is kept: %s" % (len(passes), len(fails), OUT))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


if __name__ == "__main__":
    sys.exit(main())
