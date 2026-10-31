r"""THE FAKE CLIENT AGAINST THE BUILT HELPER (2026-10-05, frame generation phase 2, stages 4 and 5). tools\test-fg-pipe\fgpipe.cpp
is a 32-bit stand-in for the game: it starts the helper with its own pid, speaks the real protocol over the real pipe and
holds the helper's shared images and fences through Vulkan with the mod's own import code. Its head says what it proves.

This script builds it 32-bit, stages the helper as built in the tree with the libraries each SESSION needs (AMD's three by
their pins, tools\ffx_pin.py; NVIDIA's from external\ngx) in folders under %TEMP%, and runs the sessions below. A session is
one helper process and one fake client; the helper's own log is read beside it.

  A  DLSS, frame generation asked (--fg=1): the whole run of stage 4 (pixels through a transport build, the control of the
     withheld in-fence, thirty frames, the helper ending by itself), the handshake says "asked and available", AND SINCE
     STAGE 6 FRAME GENERATION'S WIRE (fgpipe.cpp, --fake-fg=1): 'g' and its ack, the imports, the three constants read
     where the machine's order says (development helper), whole pairs with the generated image read by pixels, THE HELD
     'Q' and THE HELD 'G', the kinds with no frame, a repeated 'Q', a 'G' with no 'Q', a refused 'g', a second format,
     the release, twenty acquire and release cycles, a rebuild with frame generation alive, the pipe closed with it
     alive. THE HELPER'S OWN COUNT of what it read (its line for the connection) must be what this script knows the
     fake client sent, number by number, and its video memory after each of the twenty creates must be flat
  B  DLSS, not asked: neither bit; then the tag 'g' alone ends the session as an UNKNOWN TAG, as it does to a helper that
     has none of this code, and the helper's log says nothing of frame generation at all
  C  FSR (--backend=1), not asked: neither bit
  D  FSR, asked: asked and available, and what the helper says of the UPSCALERS is what C said
  E  DLSS, asked, the frame generation library moved away: 256 without 1024; then 'g' ends the session with the door's own
     reason (not offered at the handshake), not as an unknown tag
  F  DLSS, asked, no FidelityFX file at all: 256 without 1024, and the reason names the file
  G  DLSS, asked and available: a tag that is nobody's ends the session as an unknown tag (the door's "not mine")
  H  THE RAISE LEVER (development helper only, --fgraise=2: the version query raises): 256 without 1024, the whole run of
     stage 4 is still served, the upscaler side of the caps is B's, and the helper ends without unloading the library
  I  an Agility SDK runtime beside the helper (run when one is on this machine): 256 without 1024 with the reason
  L2 THE RAISE AT A DISPATCH (development, --fgraise_at=2): the pair before has its frame, the pair that raised and the
     one after have none, the upscaler's frames go on BY PIXELS, the next 'g' is refused as raised, and the helper ends
     without unloading the library
  L3 THREE FAILED DISPATCHES (development, --fgfail_at=2): the pair after them is not tried, and a new 'g' starts afresh
  L4 THE VIDEO MEMORY RULE, FORCED (development, --fgvram_refuse=1): 'g' refused with the numbers, the frames go on
  L5 LATE (development, --fgslow_g=3000): the helper sleeps before its ack and the client's own lines, given 1 s, say LATE
  L6 A BROKEN PIPE (development, --fgdie_g=1): the helper ends on 'g'; the client's own lines say so and hold nothing
SINCE STAGE 7 EVERY SET IS ACQUIRED AND RELEASED BY THE CLIENT'S OWN LINES (source/fg/fg_xport.h, the mod's code for the
'g' exchange, the imports and the release, instantiated over the fake client's binding): the four outcomes of the
exchange (created, refused, late, a broken pipe) are theirs, and so are the twenty cycles.
SINCE STAGE 10 'Q' AND 'G' ARE WRITTEN BY THE CLIENT'S OWN LINES TOO (FgxWriteQ, FgxWriteG), in every session above, and:
  W1 THE QUEUED WAIT, THE HELPER HELD IN A PREPARE (--fake-fg=7; both helpers): the watch's term over hand-fed rows, and
     since 2026-10-06 the term of the gate before DXVK's Present (IvtGateStep) over its own nine; the
     one GPU wait of the game's on a value of the helper's, queued by the client's own lines (FgxPrepWait) with a body
     behind it; THE CONTROL (no release: the body does not run); the watch's term reads the hold as ours; the client's
     release (FgxPrepRelease) lets the body run, and a second release signals nothing
  W2 THE HELPER SUSPENDED (--fake-fg=8): THE CONTROL again; the watch's term condemns it in 4 s on real readings and the
     real clock; the release is tried while it is still frozen (a fact for the record); then the watch's own order, the
     helper ended first, and the body runs
  W3 THE HELPER KILLED with a 'Q' unanswered (--fake-fg=9): MEASURED, the kernel leaves a dead helper's fences at the
     top, so the wait holds nothing and the client's release has nothing to signal
  K  THE CAPS CONTROL: session C's helper read with the expectation "asked and available" must FAIL that one proof (an
     instrument that could not read "not offered" could not prove an offer)
SINCE THE WIRE'S ONE REVISION (2026-10-06, phase 3; docs\reports\fg-3\wire-2026-10-06\specification.md, part 2) THE OFFER
IS CAPS BIT 1024 ("frame generation, wire 2"), 'Q' is 81 bytes and a pair's 'G' carries its 'Q''s reasons, in every
session above; EVERY session also proves that wire 1's bit, 512, is NOT offered, asked or not; and:
  W4 WIRE 2 (--fake-fg=10; both helpers): a set asked WITHOUT the HUD-less image (two imports, the ack's third image all
     zero, the helper's line says "two images", a pair generates from PRESENT alone, twenty cycles with the helper's
     video memory flat); a camera cut said on 'G' ALONE and one said on both, counted ONCE each by the helper; a 'Q'
     that says PIPELINED, counted; and a set WITH the image again by clearing the bit
(THE PAIR STAMP's sessions T1 to T5, from phase 3, 2026-10-06 (development helper, --fgstamp=1; host\fg_stills.h; the fake
client's mode --fake-fg=21, fgpipe_stills.h), went with the stamp in round 8's deep strip, 2026-10-26.)
SINCE 2026-10-10 THE HELPER'S TWO FIXES OWED BEFORE A GENERATED FRAME IS SHOWN (host\fg_host.h, FgAnswerOut and
FgBeginCommands; the map's helper-wire-18 and helper-wire-19; the fake client's modes --fake-fg=12 and 13, fgpipe_fix.h).
Each has a control: the same run against a helper told to do what it did until that day, which must FAIL the proofs
named for it and no other.
  R1  AN ANSWER THE CPU SIGNALS NEVER SAYS "A FRAME" (development: the queue's refusal is planted, --fgrefuse_at): every
      kind of pair with its answer made by the CPU (dispatched, withheld, no source, closed, a 'G' with no 'Q', three
      failed dispatches, not prepared, no set, a raise in AMD's library, the latch), each read BY EQUALITY as the game
      reads it: 2n+1, no frame. R1c, ITS CONTROL (--fgcpu_same=1): the dispatched pair reads 2n, "a frame"
  R2  NOTHING THE QUEUE STILL OWES MAY LAND BEHIND A CPU SIGNAL: with an older pair's answer queued behind its held
      dispatch the CPU signals nothing, and fgOut is never read lower than before. R2c, its control: the fence is read
      at the newer pair's value and then LOWER
  R3  THE DEVICE REMOVED FOR REAL behind a dispatch's submit (--fgremove_at, Direct3D's own RemoveDevice): fgOut never
      reads "a frame" for that pair, and the helper, which asks its device behind every dispatch's submit, ends the
      session there with a removed device's exit code. R3c, ITS CONTROL: the helper counts a dispatch for a list that
      never ran and goes on serving frames from the removed device
  N1  THE RING NEVER BLOCKS THE SERVE THREAD, THE REAL HOLD (the development helper on a ring of three, --fgring=3, the
      product's length until 2026-10-26): a dispatch held behind its fgIn, three pairs behind it, the last two needing
      the held list's slot. On the third build's helper the same run on the product's ring of eight, where every list
      fits and none is refused. What is measured is THE SERVE THREAD'S OWN TIME, by a message it answers over
      the pipe (nothing on the one queue can answer while it is held: session A's held 'G'). N1c, ITS CONTROL
      (--fgring_wait=1): those answers take two seconds each
  N2  EVERY SLOT HELD, THE QUEUE GOING ON (development, --fgring_hold): six served lists get no slot; each pair is
      answered "no frame" at once, fgPrep comes at once, and the frame written right behind each keeps the latency it
      has behind a pair that dispatched. N2c, ITS CONTROL: each of those times is two seconds
In every asked session the upscaler side of the caps must be what the same helper says when it is not asked: frame
generation's own load path may not change what the helper says of DLSS or FSR.
SINCE WIRE 3 (2026-10-18, the true fill, package G2; source\transport\feed_ipc.h, "WIRE 3"; the fake client's mode
--fake-fg=14, fgpipe_wire3.h) THIS HELPER OFFERS 2048 BESIDE 1024 and the fake client asks EVERY SET at wire 3 where it is
offered, as the game does: so every session above is also a proof that a set of wire 3 whose pairs ask one frame is what
wire 2's was (the counts, the pictures, the answers). And:
  X1  WIRE 3 ON THE STOCK ARM (AMD's library; both helpers): a set with no fourth image, A SET OF FOUR IMAGES (four imports,
      the helper's four constants by pixels on the development helper), the answer at count 1 (GEN alone written), THE
      STOCK ARM'S FALLBACK at counts 2 and 0 (no frame, nothing written, counted), a count of 3, a 'g' of wire 2 on this
      helper (an old client's layout: one frame whatever 'G''s bits 2 and 3 say), twenty cycles of a four-image set with
      the helper's video memory flat, and the helper's line that names the four images with GEN2's size
  X2  THE SAME HELPER, THE FAKE CLIENT TOLD TO SPEAK WIRE 2 (--fake-wire=2): wire 2's layout, one frame a pair, and nothing
      of wire 3 in the helper's lines
  X3  AN OLD CLIENT AGAINST THIS HELPER: session A's whole run by THE BASE'S OWN FAKE CLIENT, its own verdict and the helper's
      own count FG_COUNTS, and nothing of wire 3 in the helper's lines
  X4  THIS CLIENT AGAINST AN OLD HELPER, THE BASE'S OWN DEVELOPMENT HELPER: session A's whole run, and the wire 3 mode, which
      must find 1024 without 2048 and speak wire 2
      THE OLD BINARIES ARE PINNED BY COMMIT, BASE_WIRE2 (755ff449, the last commit of wire 2): built here from that commit's
      own sources (git archive; the helper by its own build-host.ps1 with -IdentityBuildId, the client by the same cl line as
      this one), once, into %TEMP%\dlssiv-test-fg-pipe-base-<commit>, and their SHA-256 printed every run. No built
      executable is repository content (the .gitignore's rule), so the commit is the pin. --old-client= and --old-helper=
      put other binaries in their place
  X5  THE OWN ARM (--own-lib=<dlssiv-fg.dll>, the helper's --fgown): the counts the set makes MUST BE 0, 1 AND 2. A library of
      interface 2, or this helper built against interface 2's header (fg_own.h's STAND-IN for interface 3), makes one, and
      X5 FAILS THERE ON PURPOSE: the counts of 0 and 2 are proved on a library of the mod's interface 3 and nowhere else
    ... --wire3                                 sessions X1 and X2 alone, and X3 to X5 where their binaries are given
  X6  THE SECOND INPUT SET (2026-10-20, round 5, H; --fake-fg=15, fgpipe_wire4.h): the helper offers 4096; a set with PRESENT2 and
      HUDLESS2, pairs by turns, each set's read told apart by pixels with its control, the sets never mixing, a set without the second;
      the helper's own counts TW_COUNTS and TW_SET; since A1 (2026-10-24) each set's guides at the render size, and W4i the guides'
      copy's cost, told. IN THE WHOLE RUN since 2026-10-24 (MIN_PROOFS), and alone with:
    ... --twin                                  session X6 alone (41 proofs on the development helper)
  WF1 to WF4  UNDER WINE AND PROTON (2026-10-24, round 7, L; --fake-fg=16 --fake-wine=N, fgpipe_wine.h): the fake client's binding says
      Wine; the game's own three fences and their probe (WF1), a silent fence planted each way (WF2 the helper sees nothing, WF3 the game
      sees nothing, with the control: the silent fence trusted holds a submit queued behind a wait on it), the bridge (WF4). IN THE
      WHOLE RUN (MIN_PROOFS), and alone with:
    ... --wine                                  sessions WF1 to WF4 alone (128 proofs on the development helper)

THE MOD'S OWN LIBRARY BY DEFAULT ON THE THIRD BUILD'S HELPER (2026-10-25; host\fg_own.h, "THE SHIPPED HELPER"). The third
build's helper is the helper the shape 1.1 ships, and it runs frame generation on dlssiv-fg.dll beside it, held to its record
(tools\ffx-own\RECORD-dlssiv-fg-cut.txt), and on nothing else. So with --rel every folder that offers frame generation is
staged with THAT library (obj\dlssiv-fg\dlssiv-fg.dll, checked against the record first) and WITHOUT AMD's frame generation
provider: every session above that is asked runs through the own library with no argument, E is the library missing, F has no
AMD file at all and frame generation IS offered, and X1 is wire 3 on the arm of three counts (X5's expectations). And:
  O1  THE DEFAULT: a folder with dlssiv-fg.dll and NOT ONE FILE OF AMD's: session A's whole run, the library's load line with
      the record's size and hash, the probe's line by the mod's own interface, and the helper's own count
  O2  THE LIBRARY MISSING: frame generation off with ONE line of the helper's, the handshake's FEED_CAP_FG_NOLIB (8192, from
      which the game writes its one warnings line), and the upscaler's whole run of frames goes on
  O3  ONE BYTE CHANGED (the same size): refused by its hash, the same three facts
  O4  ONE BYTE MORE: refused by its size, the same three facts
  O5  AMD's PROVIDER BESIDE IT, THE OWN LIBRARY MISSING: refused as O2, and AMD's file is not taken in its place
  Each of O1 to O5 FAILS on the base's own helper (--helper= names another helper for that).
    ... --own                                   sessions O1 to O5 alone (the third build's helper only)
    ... --helper=<dlssiv-helper.exe>            run against that helper instead of host\dlssiv-helper.exe (the fail-first runs)

    python tools\test-fg-pipe\run.py            the development helper (all sessions)
    python tools\test-fg-pipe\run.py --rel      the third build's helper, -Release -Fg, the one 1.1 ships (flavour word "release" since round 8; no session H: the lever is development's)
    ... --fixes                                 sessions R1 to N2c alone (the two fixes and their controls), on either helper
exit code 0 = every requirement held

THE FREEZE RUNS (2026-10-06; fgpipe_freeze.h, --fake-fg=30; docs\reports\fg-2\freeze-repro\REPORT.md). ASKED FOR BY NAME,
never part of the sessions above: they open a borderless window the size of the screen for about twenty seconds each.
On 2026-10-06 the owner's legs p2h and p2z froze the game for good after the watch ended a helper that had stopped
answering a prepare. These runs are that freeze with no game: the fake client makes a REAL D3D9 device through the stock
DXVK of external\dxvk, imports the helper's fences into DXVK's own device, queues the wait on the helper's prepare on
DXVK's own queue and KEEPS PRESENTING while this side's watch thread does what the mod's watch does. Each run is one
helper and one fake client; the main case and its controls differ in one thing each:
  main  vsync (FIFO), the helper HELD in a prepare by its own lever (--fghold_q), the watch's whole order after 4.4 s
  K1    the same hold, released after 4.4 s with the helper LEFT ALIVE (the two signals, no TerminateProcess)
  K2    main with the present interval IMMEDIATE (vsync off)
  K3    no hold and no queued wait: the helper is simply ended while presenting with vsync (leg p2x)
  K4    main with a short stall: the watch's order after 0.5 s
  K5    main with the helper SUSPENDED instead of held (leg p2z)
  K6    no frame generation at all: the helper suspended, so the wait that stands is the frame's own (the shipped path)
  E1    main with DXVK's engine name left as "DXVK" (the mod renames it "DXVK_NvRemix" for NVIDIA's driver, which then
        presents the window on another path; every other run carries the mod's name)
  H1    main with an A16B16G16R16F back buffer and scRGB, as the owner's legs had it (Windows HDR must be on)
  S1    main with THE SPY between DXVK and the driver: every vkQueuePresentKHR, vkWaitForPresentKHR and
        vkAcquireNextImageKHR with its present id, its thread, when it was called and when it returned
  T1, T2, T3   main with the watch's order after 1, 2 and 3 s: how long the queue must stand
THE GATE BEFORE DXVK'S PRESENT (source/transport/present_gate.h, IvtGateCore: the mod's one gate since the fold of 2026-10-10,
the release's core, which the product calls with its prepare fence as the second need; run here on the freeze case's fences
by --fake-pres-gate=2), against every run above that froze and every one that did not:
  G2    main with the gate                      W1  K1 with it (the helper left alive)
  W2    K2 with it (vsync off)                  W3  K3 with it (the helper simply ended: nothing for it to hold on)
  W4    K4 with it (a stall of 0.5 s)           W5  K5 with it (the helper suspended)
  W6    K6 with it (no frame generation: the frame's own wait)
  WH    H1 with it (an HDR back buffer)         WT  T1 with it (a stall of 1 s)
  WL    main with it and a stall of 7 s
  F1, WF   THE HELPER SUSPENDED BEFORE THE FIRST FRAME of the connection (it has answered nothing, every fence reads 0),
        without the gate and with it: where a gate that took a counter of 0 for "could not be read" was blind
  Q1, WQ   the helper held before its FIRST PREPARE by its lever (the first frame answered, fgPrep still 0)
  P1, WP   main in THE PIPELINED ORDER (the injection waits for the frame before it), without the gate and with it;
        NP: no stall in that order, 20 s, with the gate
  THE GATE LOOKS ONLY UNDER VSYNC, as the product's does (fg_product.h): in W2 and N2, which present with vsync off, it
        holds nothing and reads nothing, and their rows say how many presents it did not look at
  N1, N2, N3   THE GATE'S COST WHEN NOTHING STALLS: 20 s of frames with no hold at all. N1 at 59 frames a second with vsync
        and the gate; N2 with no limiter, vsync off and the gate; N3 as N2 with no gate (what N2's frames are read against)
  G1, G3, G4, G5   the STRONG gate, which is NOT the product's (test code of fgpipe_freeze.h, kept for the comparison)
A FREEZE IS A RESULT HERE, NOT A FAILURE. What must hold is the instrument: the device is DXVK's, the session ran as a
game's before the hold, the hold was made, the watch did its part, DXVK's own log names the present mode the run is for.
EVERY RUN HAS A HARD DEADLINE: at "freeze-snapshot-wanted", or 90 s after its start, this script reads every thread of the
fake client (tools\hangstacks.py), ends the fake client and its helper BY THEIR PIDS (never by name: a game may be
running) and goes on. At the end it checks that none of the processes it started is left.

    python tools\test-fg-pipe\run.py --freeze                 every run above, once
    python tools\test-fg-pipe\run.py --freeze=main,K1         the named ones
    python tools\test-fg-pipe\run.py --freeze=main --times=3  each named run three times
    ... --quiet-window                                        every run in a 480x270 window in the bottom right corner that is
                                                              not topmost and never takes the foreground (somebody is using the screen)
exit code 0 = the instrument held in every run (whatever each run's verdict)

THE STANDING PROOF OF THE GATE (2026-10-06). Two runs, and here the verdict IS held:
  the main case WITH the gate must be RECOVERED, with the gate having held EXACTLY ONE present and given up on none;
  the main case WITHOUT it, once, is the control and is INFORMATIONAL: FROZEN is what this machine's driver gave on
  2026-10-06. If it reads RECOVERED the driver no longer shows the fault: that is said, and it fails nothing (the gate
  is then proved to do no harm, and no longer proved to be what saves the run).
It ends with PASS or FAIL and leaves no process behind. NOT part of tools\prove-all.py: it opens a window.

    python tools\test-fg-pipe\run.py --freeze-proof --quiet-window
exit code 0 = the gate held as it must

(Until the fold of 2026-10-10 the gate these runs held was fg_xport.h's FgxPresentGate, and a switch, --release-gate, built
the fake client on the release line's core instead: the proof passed on both with the same verdict in every case. The
release's core is the only one now and the switch is gone.)
"""
import bisect, hashlib, os, queue, re, shutil, subprocess, sys, tempfile, threading, time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))
import ffx_pin

HELPER = next((a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith("--helper=")), None) or os.path.join(ROOT, "host", "dlssiv-helper.exe")
REL = "--rel" in sys.argv[1:]
OWN = "--own" in sys.argv[1:]                       # sessions O1 to O5 alone (the head says what they are)
# THE MOD'S OWN LIBRARY (2026-10-25): what --rel stages beside the helper, held to its record first (own_record)
OWN_DLL = os.path.join(ROOT, "obj", "dlssiv-fg", "dlssiv-fg.dll")
OWN_RECORD = os.path.join(ROOT, "tools", "ffx-own", "RECORD-dlssiv-fg-cut.txt")
FREEZE_PROOF = "--freeze-proof" in sys.argv[1:]    # the standing proof of the gate before DXVK's Present (the head says what it holds)
FIXES = "--fixes" in sys.argv[1:]                   # the two fixes' sessions alone (the head says what they are)
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-pipe" + ("-rel" if REL else "") + ("-own" if "--own" in sys.argv[1:] else "") + ("-fixes" if FIXES else "") + ("-wire3" if "--wire3" in sys.argv[1:] else "") + ("-twin" if "--twin" in sys.argv[1:] else "") + ("-wine" if "--wine" in sys.argv[1:] else "") + ("-freeze-proof" if FREEZE_PROOF else "-freeze" if any(a == "--freeze" or a.startswith("--freeze=") for a in sys.argv[1:]) else ""))
FLAVOUR = b" release, backend" if REL else b" development, backend"
NGX = os.path.join(ROOT, "external", "ngx", "nvngx_dlss.dll")
AGILITY = os.environ.get("DLSSIV_AGILITY_CORE", "")   # an Agility SDK D3D12Core.dll; without one session I is not run
# THE FLOORS: proofs of the fake client's plus this script's own, WITHOUT session I's (it runs only where an Agility runtime
# is on the machine). Raised in the commit that adds proofs; a run with fewer is a proof that stopped running.
MIN_PROOFS = 918 if not REL else 742        # (ROUND 8'S DEEP STRIP, 2026-10-26: the pair stamp's sessions T1 to T5 out with host\fg_stills.h, 1052 to 918 on the development helper as its first whole run without them counted, 0 failed; the third build's floor is unmoved, no T session ran there.) (THE MOD'S OWN LIBRARY BY DEFAULT, 2026-10-25: the third build's floor 427 to 742, its first whole run since X6 and WF1 to WF4 joined it, with sessions O1 to O5 and X1's new proof: 741 passed and X1's one failed, mended and passed alone with --wire3.) (ROUND 7, L, 2026-10-24: WF1 to WF4, under Wine the game's own fences, in the whole run: 924 to 1052 on the development helper, their 128 as --wine counted them; the third build's floor stands at 427 until a whole --rel run counts them there.) (ROUND 5, H, 2026-10-24: X6, the second input set, in the whole run: 883 to 924 on the development helper, its 41 as --twin counted them; the third build's floor stands at 427 until a whole --rel run counts X6 there.) (WIRE 3, round 2, 2026-10-18: X3 and X4 run every time on the base's own binaries, built from 755ff449: 808 to 883 on the development helper (X3 4, X4 44, X4b 27; --wire3 alone counted 148, X1 to X4b), 354 to 427 on the third build's (--rel --wire3 alone counted 145: X1 and X2's 72 and these 73). Before that, WIRE 3, 2026-10-18: 735 to 808 on the development helper, sessions X1 and X2's 73 (X1 34 of the fake client's and 9, X2 25 and 5: the first whole run counted 883 with X3, X4 and X4b's 75 more, which run only where their old binaries are given); 282 to 354 on the third build's, X1 33 and 9, X2 25 and 5, as its first whole run counted them (354). Before that: THE HELPER'S TWO FIXES, 2026-10-10: 562 to 735 on the development helper, sessions R1 to N2c's 173 (MIN_FIXES); 250 to 282 on the third build's, session N1's 32, the real hold on its own ring of three. Before that: LEVER 2, 2026-10-10: 532 to 562 on the development helper, session E1's thirty as its first whole run counted them ('Q' right behind 'F' with the helper held ahead of that prepare: the frame's own answer does not wait); the third build's floor stands, E1's hold is a development lever. Before that: THE WIRE'S ONE REVISION, 2026-10-06: 346 to 397 and 206 to 250: "512 is not offered" in every session, and session W4's proofs; THE UNION with the pair stamp's sessions, 2026-10-07: 532 on the development helper as its first whole run counted them: 397, T1 to T4's 97 and their four "512 is not offered", and T5's 34, the stamp on a set with no HUD-less image; 250 stays on the third build's, where no T session runs) stage 10 (2026-10-06): 245 and 105, and the 88 of sessions W1, W2 and W3; the review of phase 2: 4 more (the watch's term on the game's own state) on each helper; the gate's term over its hand-fed rows: 9 more on each (8, and since the review of the gate the row for a counter read as 0); phase 3, the pair stamp: 97 more on the development helper (346 before: T1 34, T2 31, T3 31, T4 1)
WAIT_RUN = {7: 25, 8: 6, 9: 5}             # the fake client's proofs more than a whole run's, in each of stage 10's modes (mode 7: the watch's rows, and since the gate before DXVK's Present its nine)
FULL_RUN = 18        # the fake client's proofs in a whole run with --fake-want-fg (stage 4's sixteen, the handshake's, and since wire 2 "512 is not offered")
FG_RUN = 21          # more of them with --fake-fg=1 (stages 6 and 7), and two more with --fake-fg-stamp
# WHAT THE FAKE CLIENT SENDS IN --fake-fg=1, AS THE HELPER MUST HAVE COUNTED IT (fgpipe.cpp, FgRun: the pairs are 33 to 38
# and 46 to 51, 'Q' 37 is sent twice, 'G' 45 has no 'Q'; the sets are the first, the second format, twenty cycles, the
# one before the rebuild and the one after it; two 'g' are refused for their format, one sent by hand with a set still
# held and one through the client's own lines).
FG_COUNTS = "24 set(s) built, 2 refused; 'Q' 13, 'G' 13, dispatched 9, answered with no frame 4; DUPLICATE 1, DRIFT 1; resets by reason: cut 0, build 3, device 0, gap 2, the helper's own 1; pipelined 0"
# WIRE 2'S SESSION (--fake-fg=10, fgpipe.cpp's Wire2Run): its proofs more than a whole run's (one more with --fake-fg-stamp),
# and what the helper must have counted: 22 sets (the first, twenty cycles, the three-image one), pairs 33 to 39, a cut
# said on 'G' alone (35) and one on both (36) counted ONCE each, the two sets' first pairs, one 'Q' PIPELINED (37)
W2_RUN = 10
W2_COUNTS = "22 set(s) built, 0 refused; 'Q' 7, 'G' 7, dispatched 7, answered with no frame 0; DUPLICATE 0, DRIFT 0; resets by reason: cut 2, build 2, device 0, gap 0, the helper's own 0; pipelined 1"
W2_FRAMES = 39       # stage 4's 32, and 33 to 39
# LEVER 2'S SESSION (--fake-fg=11, fgpipe.cpp's EarlyQRun; the development helper, held by --fghold_q=2): its proofs more
# than a whole run's, and what the helper must have counted: one set, pairs 33 to 35, all dispatched
EQ_RUN = 5
EQ_COUNTS = "1 set(s) built, 0 refused; 'Q' 3, 'G' 3, dispatched 3, answered with no frame 0; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 0; pipelined 0"
EQ_FRAMES = 35       # stage 4's 32, and 33 to 35
# WIRE 3'S SESSIONS (2026-10-18; --fake-fg=14, fgpipe_wire3.h): the fake client's proofs more than a whole run's (one more with
# --fake-fg-stamp), what the helper must have counted on each arm, and the frames. ON THE STOCK ARM (X1): 24 sets (the one
# without the fourth image, the four-image one, the wire 2 one, twenty cycles and the last), pairs 33 to 39, dispatched 33,
# 34, 37 and 39; 35 (two), 36 (flow-only) and 38 (three) not made; build 2 (33 and 39); the helper's own reset at 37, after
# two pairs with no frame. ON AN ARM THAT MAKES THE THREE COUNTS (X5): 35 dispatched with two, 36 flow-only and kept (no
# frame, and 37 is NOT a reset of the helper's own), 38 alone not made.
W3_RUN = 14
W3_RUN_O = 6         # the wire 2 path of the same mode (X2, X4)
W3_COUNTS = {"stock": "24 set(s) built, 0 refused; 'Q' 7, 'G' 7, dispatched 4, answered with no frame 3; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 2, device 0, gap 0, the helper's own 1; pipelined 0",
             "three": "24 set(s) built, 0 refused; 'Q' 7, 'G' 7, dispatched 5, answered with no frame 2; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 2, device 0, gap 0, the helper's own 0; pipelined 0",
             "wire2": "1 set(s) built, 0 refused; 'Q' 3, 'G' 3, dispatched 3, answered with no frame 0; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 0; pipelined 0"}
W3_MORE = {"stock": "flow-only pairs 0, pairs of two frames 0, pairs that asked a count their set does not make 3",
           "three": "flow-only pairs 1, pairs of two frames 1, pairs that asked a count their set does not make 1"}
W3_FRAMES, W3_FRAMES_O = 39, 35
WIRE3 = "--wire3" in sys.argv[1:]
# THE SECOND INPUT SET'S SESSION (2026-10-20, round 5, H; --fake-fg=15, fgpipe_wire4.h; run alone with --twin): the fake client's proofs
# more than a whole run's (one more with --fake-fg-stamp), and what the helper must have counted: 24 sets (the one with both sets, the
# wire 3 one, the one with no HUD-less image, twenty cycles and the last), pairs 33 to 39; dispatched 33, 34, 35, 37 and 39; 36 (the
# 'Q' in the second set, the 'G' in the first) and 38 (the second set named in a set without it) with no frame; build 2 (33 and 38);
# the helper's own resets at 37 and 39, each behind a pair with no frame. The development helper's own line of the second set: pairs
# in it 2 (34 and 36), named in a set without it 1 (38), dispatches whose 'G' named the other set 2 (36, and 38's 'G')
TW_RUN = 13
TW_COUNTS = "24 set(s) built, 0 refused; 'Q' 7, 'G' 7, dispatched 5, answered with no frame 2; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 2, device 0, gap 0, the helper's own 2; pipelined 0"
TW_SET = "pairs in it 2; pairs that named it in a set without it 1; dispatches whose 'G' named the other set 2"
TW_FRAMES = 39
TWIN = "--twin" in sys.argv[1:]
# UNDER WINE AND PROTON (2026-10-24, round 7, L; --fake-fg=16, fgpipe_wine.h; alone with --wine): each case's proofs
# more than a whole run's (the fake client's own count of its Proof lines in that case, the exit's top included where a set holds the
# fences), and the frames each sends (WF1 its pairs 33 to 36, the others the upscaler's 33 to 35)
WN_RUN = {1: 9, 2: 8, 3: 10, 4: 4}
WN_FRAMES = {1: 36, 2: 35, 3: 35, 4: 35}
WINE = "--wine" in sys.argv[1:]
def _arg(name):
    return next((a.split("=", 1)[1] for a in sys.argv[1:] if a.startswith(name + "=")), None)
OLD_CLIENT, OLD_HELPER, OWN_LIB = _arg("--old-client"), _arg("--old-helper"), _arg("--own-lib")
BASE_WIRE2 = "755ff449"   # the last commit of wire 2: X3's client and X4's helper are built from its sources (base_binaries)


def base_binaries(vcvars):
    """The fake client and the development helper of BASE_WIRE2, built from that commit's own sources once (and kept): (client,
    helper), or (None, why)."""
    import io, tarfile
    d = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-pipe-base-" + BASE_WIRE2)
    src = os.path.join(d, "src", "DLSS-IV")
    client, helper, done = os.path.join(d, "fgpipe.exe"), os.path.join(src, "host", "dlssiv-helper.exe"), os.path.join(d, "BUILT")
    if not (os.path.exists(client) and os.path.exists(helper) and os.path.exists(done)):
        shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
        # (from the top of the work tree: a pathspec is the root's there, and in a subfolder git keeps only that subfolder)
        g = subprocess.run(["git", "-C", os.path.dirname(os.path.dirname(ROOT)), "archive", "--format=tar", BASE_WIRE2, "src/DLSS-IV"], capture_output=True)
        if g.returncode != 0: return None, "git archive %s failed: %s" % (BASE_WIRE2, g.stderr.decode(errors="replace")[:200])
        with tarfile.open(fileobj=io.BytesIO(g.stdout)) as t:
            t.extractall(d, members=[m for m in t.getmembers() if not m.name.startswith("src/DLSS-IV/docs/")], filter="data")
        os.makedirs(os.path.join(src, "external", "ngx", "lib"), exist_ok=True)
        for n in os.listdir(os.path.join(ROOT, "external", "ngx", "lib")):      # the git-ignored import library the build needs
            shutil.copy2(os.path.join(ROOT, "external", "ngx", "lib", n), os.path.join(src, "external", "ngx", "lib", n))
        b = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(src, "build-host.ps1"), "-IdentityBuildId", BASE_WIRE2],
                           capture_output=True, text=True, errors="replace", cwd=src)
        if b.returncode != 0 or not os.path.exists(helper): return None, "the base's helper did not build: " + b.stdout[-400:]
        cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /DWIN32 /I"%s" /I"%s" /I"%s" "%s" /Fe:fgpipe.exe /Fo:fgpipe.obj /link /INCREMENTAL:NO'
              % (vcvars, os.path.join(src, "external", "vulkan"), os.path.join(src, "source", "transport"), os.path.join(src, "source", "fg"), os.path.join(src, "tools", "test-fg-pipe", "fgpipe.cpp")))
        r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=d)
        if r.returncode != 0 or not os.path.exists(client): return None, "the base's fake client did not build: " + r.stdout[-400:]
        open(done, "w").write(BASE_WIRE2 + "\n")
    for p in (client, helper):
        print("  the base's (%s) %s: sha256 %s" % (BASE_WIRE2, os.path.basename(p), hashlib.sha256(open(p, "rb").read()).hexdigest()))
    return client, helper
# THE TWO FIXES' SESSIONS (--fake-fg=12 and 13, fgpipe_fix.h; the head says what R1 to N2c are): the fake client's proofs
# more than a whole run's in each case, what the helper must have counted of each, and the floor of these sessions alone
FIX_RUN = {"R1": 15, "R2": 7, "R3": 5, "N1": 8, "N2": 10}
FIX_COUNTS = {
    "R1": "2 set(s) built, 0 refused; 'Q' 12, 'G' 13, dispatched 1, answered with no frame 12; DUPLICATE 0, DRIFT 1; resets by reason: cut 0, build 2, device 0, gap 1, the helper's own 0; pipelined 0",
    "R2": "1 set(s) built, 0 refused; 'Q' 5, 'G' 5, dispatched 4, answered with no frame 1; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 1; pipelined 0",
    "R3": "1 set(s) built, 0 refused; 'Q' 2, 'G' 2, dispatched 1, answered with no frame 1; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 0; pipelined 0",
    "N1": "1 set(s) built, 3 refused; 'Q' 7, 'G' 7, dispatched 5, answered with no frame 2; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 1; pipelined 0",
    "N2": "1 set(s) built, 0 refused; 'Q' 16, 'G' 16, dispatched 10, answered with no frame 6; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 1; pipelined 0"}
FIX_MORE = {"R1": (12, 0), "R2": (1, 0), "R3": None, "N1": (0, 2), "N2": (0, 6)}   # answers the queue refused, lists the ring refused; None: no such line (both are zero)
# THE PRODUCT'S RING IS EIGHT LONG (2026-10-26, host\fg_host.h, FG_RING: it was three until then): on the third build's helper N1's
# held dispatch and the three pairs behind it fit, so no list is refused and the line of the two fixes' counts is not said (the
# connection's count is the same: pairs 36 and 37 are NO SOURCE, no frame on either length, and the helper's own reset is pair
# 38's). The development helper still runs N1 on a ring of three (--fgring=3), where the refusal is planted.
if REL:
    FIX_MORE["N1"] = None
MIN_FIXES = 173 if not REL else 32         # the floor of --fixes alone, as its first whole runs counted them (2026-10-10): 173 on the development helper (ten sessions, five of them controls), N1's 32 on the third build's
FG_FRAMES = 44       # frame messages in session A: stage 4's 32, and 33 to 38 and 46 to 51
HELLO_RUN = 7        # in a session that ends at the hello or at one tag (six, and since wire 2 "512 is not offered")
# THE FREEZE RUNS (see the head): asked for by name, development helper only (the main case's hold is its lever)
FREEZE = next((a for a in sys.argv[1:] if a == "--freeze" or a.startswith("--freeze=")), None)
FREEZE_TIMES = next((int(a.split("=", 1)[1]) for a in sys.argv[1:] if a.startswith("--times=")), 1)
FREEZE_QUIET = ["--fake-pres-win=480x270", "--fake-pres-top=0", "--fake-pres-quiet=1"] if "--quiet-window" in sys.argv[1:] else []
FREEZE_AT = 300      # the frame of the hold: about five seconds of frames first
FREEZE_DEADLINE = 90 # seconds a run may take before this script snapshots it and ends it
DXVK = os.path.join(ROOT, "external", "dxvk", "x32", "d3d9.dll")
# what the game's DXVK is told (GTAIV1080 - Copy\plugins\GTAIV_d3d9.log, "Effective configuration", 2026-10-06): the
# owner's dxvk.conf, the four lines of DXVK's built-in profile for GTAIV.exe (this program has another name, so they
# are written out), and the two the mod adds through DXVK_CONFIG. d3d9.forceRefreshRate is left out: it is the owner's
# display's, and DXVK uses it for the mode list alone. The present interval is the run's.
FREEZE_CONF = ("d3d9.presentInterval = %d\nd3d9.maxFrameRate = 0\ndxvk.enableGraphicsPipelineLibrary = Auto\ndxvk.trackPipelineLifetime = Auto\ndxvk.numCompilerThreads = 0\n"
               "d3d9.hideNvidiaGpu = True\nd3d9.supportDFFormats = False\nd3d9.deviceLossOnFocusLoss = True\ndxgi.maxDeviceMemory = 128\n")
FREEZE_ENV = "d3d9.maxFrameLatency = 1;d3d9.deviceLocalConstantBuffers = False"
HOLD = ["--fg=1", "--fghold_q=%d" % FREEZE_AT]
FREEZE_RUNS = [   # id, what it is, the helper's arguments, the fake client's, the present interval, frame generation asked
    ("main", "vsync, the helper HELD in a prepare by its lever, the watch's whole order after 4.4 s", HOLD, [], 1, 3),
    ("K1", "the same hold, released after 4.4 s with the helper LEFT ALIVE", HOLD, ["--fake-pres-end=2"], 1, 3),
    ("K2", "main with the present interval IMMEDIATE (vsync off)", HOLD, ["--fake-pres-interval=0"], 0, 3),
    ("K3", "no hold, no queued wait: the helper simply ended while presenting with vsync (leg p2x)", ["--fg=1"], ["--fake-pres-hold=0"], 1, 3),
    ("K4", "main with a short stall: the watch's order after 0.5 s", HOLD, ["--fake-pres-stall=500"], 1, 3),
    ("K5", "main with the helper SUSPENDED instead of held (leg p2z)", ["--fg=1"], ["--fake-pres-hold=2"], 1, 3),
    ("K6", "no frame generation: the helper suspended, the wait that stands is the frame's own (the shipped path)", [], ["--fake-pres-hold=2", "--fake-pres-fg=0"], 1, 0),
    ("E1", "main with DXVK's engine name left as \"DXVK\" (the driver's other present path)", HOLD, ["--fake-pres-engine=DXVK"], 1, 3),
    ("H1", "main with an A16B16G16R16F back buffer and scRGB, as the owner's legs had it", HOLD, ["--fake-pres-hdr=1"], 1, 3),
    ("S1", "main with the spy between DXVK and the driver", HOLD, ["--fake-pres-spy=1"], 1, 3),
    # THE STRONG GATE (--fake-pres-gate=1): NOT the product's. Test code of fgpipe_freeze.h, kept for the comparison
    ("G1", "main with THE STRONG GATE (not the product's): no present is issued behind an outstanding wait", HOLD, ["--fake-pres-gate=1"], 1, 3),
    ("G3", "K6 (the frame's own wait, the shipped path, the helper suspended) with the strong gate", [], ["--fake-pres-hold=2", "--fake-pres-fg=0", "--fake-pres-gate=1"], 1, 0),
    ("G4", "K5 (the helper suspended, frame generation on) with the strong gate", ["--fg=1"], ["--fake-pres-hold=2", "--fake-pres-gate=1"], 1, 3),
    ("G5", "K1 (released with the helper LEFT ALIVE) with the strong gate", HOLD, ["--fake-pres-end=2", "--fake-pres-gate=1"], 1, 3),
    # THE GATE, THE PRODUCT'S (--fake-pres-gate=2: the mod's one gate, IvtGateCore): at most one present behind an
    # outstanding wait. Against every run that froze, then against the three that did not
    ("G2", "main with THE GATE (the mod's one gate): at most one present behind an outstanding wait", HOLD, ["--fake-pres-gate=2"], 1, 3),
    ("W1", "K1 (released with the helper LEFT ALIVE) with the gate", HOLD, ["--fake-pres-end=2", "--fake-pres-gate=2"], 1, 3),
    ("W5", "K5 (the helper suspended, frame generation on) with the gate", ["--fg=1"], ["--fake-pres-hold=2", "--fake-pres-gate=2"], 1, 3),
    ("W6", "K6 (the frame's own wait, the shipped path, the helper suspended) with the gate", [], ["--fake-pres-hold=2", "--fake-pres-fg=0", "--fake-pres-gate=2"], 1, 0),
    ("WH", "H1 (an A16B16G16R16F back buffer and scRGB) with the gate", HOLD, ["--fake-pres-hdr=1", "--fake-pres-gate=2"], 1, 3),
    ("WT", "T1 (the watch's order after 1 s) with the gate", HOLD, ["--fake-pres-stall=1000", "--fake-pres-gate=2"], 1, 3),
    ("WL", "main with the gate and a LONG stall: the watch's order after 7 s (inside the helper's 8 s sleep)", HOLD, ["--fake-pres-stall=7000", "--fake-pres-gate=2"], 1, 3),
    ("W2", "K2 (vsync off, which recovers without it) with the gate", HOLD, ["--fake-pres-interval=0", "--fake-pres-gate=2"], 0, 3),
    ("W3", "K3 (no hold: the helper simply ended) with the gate: nothing for it to hold on", ["--fg=1"], ["--fake-pres-hold=0", "--fake-pres-gate=2"], 1, 3),
    ("W4", "K4 (a short stall, 0.5 s, which recovers without it) with the gate", HOLD, ["--fake-pres-stall=500", "--fake-pres-gate=2"], 1, 3),
    # THE HELPER STOPS BEFORE IT HAS ANSWERED ANYTHING (2026-10-06, the review of the gate): the fences still read what they
    # were created with, 0, and a gate that took a counter of 0 for "could not be read" was blind exactly here
    ("F1", "the helper SUSPENDED BEFORE THE FIRST FRAME of the connection (it has answered nothing: every fence reads 0), no gate", ["--fg=1"], ["--fake-pres-hold=2", "--fake-pres-at=1"], 1, 3),
    ("WF", "F1 with the gate: the helper suspended before the first frame", ["--fg=1"], ["--fake-pres-hold=2", "--fake-pres-at=1", "--fake-pres-gate=2"], 1, 3),
    ("Q1", "the helper HELD BEFORE ITS FIRST PREPARE by its lever (the first frame answered, fgPrep still 0), no gate", ["--fg=1", "--fghold_q=1"], ["--fake-pres-at=1"], 1, 3),
    ("WQ", "Q1 with the gate: the helper held before its first prepare", ["--fg=1", "--fghold_q=1"], ["--fake-pres-at=1", "--fake-pres-gate=2"], 1, 3),
    # THE PIPELINED ORDER (--fake-pres-order=1; the review of the gate): the body's own submit waits for the frame before it,
    # the message goes after the submit, no home batch. The order every session of the game starts in
    ("P1", "main in the PIPELINED order (the injection waits for the frame before it), no gate", HOLD, ["--fake-pres-order=1"], 1, 3),
    ("WP", "P1 with the gate: main in the pipelined order", HOLD, ["--fake-pres-order=1", "--fake-pres-gate=2"], 1, 3),
    ("NP", "NO STALL, 20 s at 59 frames a second with vsync in the PIPELINED order, with the gate", ["--fg=1"], ["--fake-pres-hold=3", "--fake-pres-after=20000", "--fake-pres-order=1", "--fake-pres-gate=2"], 1, 3),
    # THE GATE'S COST WHEN NOTHING STALLS (--fake-pres-hold=3): 20 s of frames from frame 300 on, nothing held, nobody ended
    ("N1", "NO STALL, 20 s at 59 frames a second with vsync, with the gate: what it holds in a normal frame", ["--fg=1"], ["--fake-pres-hold=3", "--fake-pres-after=20000", "--fake-pres-gate=2"], 1, 3),
    ("N2", "NO STALL, 20 s with no limiter and vsync off, with the gate", ["--fg=1"], ["--fake-pres-hold=3", "--fake-pres-after=20000", "--fake-pres-fps=0", "--fake-pres-interval=0", "--fake-pres-gate=2"], 0, 3),
    ("N3", "NO STALL, 20 s with no limiter and vsync off, with NO gate (what N2's frames are read against)", ["--fg=1"], ["--fake-pres-hold=3", "--fake-pres-after=20000", "--fake-pres-fps=0", "--fake-pres-interval=0"], 0, 3),
    ("T1", "main with the watch's order after 1 s (how long must the queue stand?)", HOLD, ["--fake-pres-stall=1000"], 1, 3),
    ("T2", "main with the watch's order after 2 s", HOLD, ["--fake-pres-stall=2000"], 1, 3),
    ("T3", "main with the watch's order after 3 s", HOLD, ["--fake-pres-stall=3000"], 1, 3),
]

passes, fails = [], []


def proof(ok, sid, text):
    (passes if ok else fails).append((sid, text))
    print("  %s  [%s] %s" % ("PASS" if ok else "FAIL", sid, text))
    return ok


def caps_bits():
    t = open(os.path.join(ROOT, "source", "transport", "feed_ipc.h"), encoding="utf-8", errors="replace").read()
    b = {m.group(1): int(m.group(2)) for m in re.finditer(r"(?m)^#define FEED_CAP_(\w+)\s+(\d+)ull", t)}
    for need in ("DLSS_PROBED", "DLSS", "FSR_PROBED", "FSR", "FG_PROBED", "FG", "FG_WIRE3"):
        if need not in b: raise SystemExit("FAIL: feed_ipc.h has no FEED_CAP_%s in the form this script reads" % need)
    return b


def place(src, dst_dir, name=None):
    """A hard link where the volume allows it (the libraries are 130 MB a folder), a copy where it does not."""
    dst = os.path.join(dst_dir, name or os.path.basename(src))
    try: os.link(src, dst)
    except OSError: shutil.copy2(src, dst)


def own_record():
    """(size, sha256) of the own library's record, each read by its key at the start of a line, and the staged library's own
    facts checked against them: SystemExit with a FAIL line on any mismatch."""
    t = open(OWN_RECORD, encoding="ascii").read()
    size = re.findall(r"(?m)^size (\d+)$", t); sha = re.findall(r"(?m)^sha256 ([0-9a-f]{64})$", t)
    if len(size) != 1 or len(sha) != 1: raise SystemExit("FAIL: %s does not hold one size and one sha256 line" % OWN_RECORD)
    if not os.path.exists(OWN_DLL): raise SystemExit("FAIL: %s is not built (build-ffx-own.ps1 -Target dlssiv-fg)" % OWN_DLL)
    b = open(OWN_DLL, "rb").read()
    if len(b) != int(size[0]) or hashlib.sha256(b).hexdigest() != sha[0]:
        raise SystemExit("FAIL: %s is %d bytes, sha256 %s; its record says %s, %s: a library nobody has checked is not staged" % (OWN_DLL, len(b), hashlib.sha256(b).hexdigest(), size[0], sha[0]))
    return int(size[0]), sha[0]


def stage(folder, ffx, ngx=True, agility=False, own=None):
    """own: None nothing of the mod's library; "lib" the library as its record holds it; "byte" with one byte changed; "long"
    with one byte more (the last two for sessions O3 and O4)."""
    d = os.path.join(OUT, folder)
    os.makedirs(d)
    shutil.copy2(HELPER, os.path.join(d, "dlssiv-helper.exe"))
    for n in ffx:
        why = ffx_pin.check(n)
        if why: raise SystemExit("FAIL: %s: a library nobody has checked is not staged" % why)
        place(os.path.join(ffx_pin.FFX, n), d)
    if own:
        own_record()
        b = bytearray(open(OWN_DLL, "rb").read())
        if own == "byte": b[len(b) // 2] ^= 0xFF
        if own == "long": b += b"\x00"
        open(os.path.join(d, "dlssiv-fg.dll"), "wb").write(bytes(b))
    if ngx: place(NGX, d)
    if agility:
        os.makedirs(os.path.join(d, "D3D12"))
        shutil.copy2(AGILITY, os.path.join(d, "D3D12", "D3D12Core.dll"))
    return d


def session(sid, title, folder, helper_args, fake_args):
    print("SESSION %s: %s" % (sid, title))
    log = os.path.join(OUT, "helper-%s.log" % sid)
    # (2026-10-31, strip item 11: the shipped helper has no transport copy, so --rel asks a real build; an old helper still has it)
    real = ["--fake-real-upscale"] if REL and folder != "oldhelper" else []
    cmd = [os.path.join(OUT, "fgpipe.exe"), os.path.join(OUT, folder, "dlssiv-helper.exe"), log] + fake_args + real + helper_args
    try:
        g = subprocess.run(cmd, capture_output=True, text=True, errors="replace", cwd=os.path.join(OUT, folder), timeout=240)
        rc, out = g.returncode, g.stdout
    except subprocess.TimeoutExpired:
        subprocess.run(["taskkill", "/F", "/IM", "fgpipe.exe"], capture_output=True)
        rc, out = 99, "  FAIL  the fake client did not finish in 240 s\nFAIL\n"
    lines = out.splitlines()
    for l in lines: print("    " + l)
    s = {"sid": sid, "rc": rc, "lines": lines,
         "p": [l for l in lines if l.startswith("  PASS  ")], "f": [l for l in lines if l.startswith("  FAIL  ")],
         "last": lines[-1].strip() if lines else "", "log": open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""}
    m = re.search(r"the hello is answered: version \d+, caps 0x([0-9A-Fa-f]+)", out)
    s["caps"] = int(m.group(1), 16) if m else None
    s["closing"] = re.findall(r"pipe closed by (.+?) after (\d+) frames \((\d+) failed\)", s["log"])
    return s


def clean(s, want):
    """The fake client's own verdict for a session that must pass: its exit code, its last line, no FAIL line, and at
    least the proofs that kind of session has."""
    ok = s["rc"] == 0 and s["last"] == "PASS" and not s["f"] and len(s["p"]) >= want
    proof(ok, s["sid"], "the fake client's own verdict: %d proofs passed, %d failed (at least %d wanted), exit code %d" % (len(s["p"]), len(s["f"]), want, s["rc"]))
    for l in s["p"]: passes.append((s["sid"], l))
    for l in s["f"]: fails.append((s["sid"], l))
    return ok


def says(s, pattern, what, want=True):
    hit = re.search(pattern, s["log"]) is not None
    return proof(hit == want, s["sid"], "the helper's log %s: %s" % ("says" if want else "does NOT say", what))


def closed_by(s, who, frames):
    c = s["closing"]
    ok = len(c) == 1 and c[0][0] == who and (frames is None or c[0][1] == str(frames)) and c[0][2] == "0"
    return proof(ok, s["sid"], "the helper's closing line: %s" % (("by %s after %s frames (%s failed)" % c[0]) if c else "NOT THERE"))


# ---------------------------------------------------------------------------------------------------------------------
# THE FREEZE RUNS (the head says what they are)
# ---------------------------------------------------------------------------------------------------------------------
# Rows of DXVK's d3d9.dll that the owner's frozen game showed too (docs\reports\fg-2\sitting-1\freeze; the game's vulkan.dll
# is this same file, so the offsets are the same). "read as" is a reading by where the row sits; the two without it were
# seen with the spy's own function on the same stack (run S1).
DXVK_ROWS = {"1FE59B": "DXVK's frame thread, inside vkWaitForPresentKHR", "1FDE53": "DXVK's frame thread, idle",
             "3EC4B": "asleep inside DXVK's D3D9 Present (read as its frame latency wait)",
             "205EDF": "read as DXVK's queue thread, idle", "2056B9": "read as DXVK's queue thread, waiting for a submission",
             "2070E3": "read as DXVK's submit thread, idle", "1563DC": "read as DXVK's CS thread, idle", "155700": "waiting for DXVK's CS thread", "155354": None}


def map_functions(path):
    """The fake client's own functions from its linker map, as sorted (rva, name): code symbols only."""
    base, syms = 0x400000, []
    for line in open(path, errors="replace"):
        m = re.search(r"Preferred load address is ([0-9A-Fa-f]+)", line)
        if m: base = int(m.group(1), 16)
        m = re.match(r"\s*0001:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})\s+f\s", line)
        if m:
            n = re.match(r"\?([A-Za-z_0-9]+)@", m.group(1))
            syms.append((int(m.group(2), 16) - base, n.group(1) if n else m.group(1).lstrip("_")))
    syms.sort()
    return syms


def snapshot_summary(text, syms):
    """One line a thread of a hangstacks.py snapshot of the fake client: what it waits in, DXVK's rows (a scan: stale rows
    appear, read the first ones), the NVIDIA driver's rows above the first of them, and this program's own functions."""
    rv = [s[0] for s in syms]
    mine = ("PPresenter", "PWatch", "PresMain", "PSpyWaitForPresent", "PSpyQueuePresent", "PSpyAcquire", "PSubmit", "PMarker", "PSignalOutTo", "PStop")
    out, quiet = [text.split("\n")[0]], 0
    for b in re.split(r"\n(?=thread \d+)", text)[1:]:
        lines = b.strip().split("\n")
        m = re.match(r"thread (\d+)\s+eip \S+\s+(.*?)\s+esp", lines[0])
        if not m: continue
        wait = m.group(2).split("  ")[-1]
        rows = lines[1:]
        dx = [x.group(1) for x in (re.search(r" d3d9\.dll\+0x([0-9A-F]+)", l) for l in rows) if x]
        dx = [x for x in dx if x not in ("290460", "29047D")]
        first = next((i for i, l in enumerate(rows) if " d3d9.dll+0x" in l), len(rows))
        drv = [x.group(1) for x in (re.search(r"nvoglv32\.dll\+0x([0-9A-F]+)", l) for l in rows[:first]) if x]
        fn = []
        for l in rows:
            x = re.search(r"fgpipe\.exe\+0x([0-9A-Fa-f]+)", l)
            if not x or not syms: continue
            j = bisect.bisect_right(rv, int(x.group(1), 16)) - 1
            if j >= 0 and syms[j][1] in mine and syms[j][1] not in fn: fn.append(syms[j][1])
        if not dx and not fn and not (drv and "DelayExecution" in wait): quiet += 1; continue
        if "PresMain" in fn:      # the main thread: its stack holds the text it printed, and text can read as an address
            out.append("  thread %-6s in %-36s this program's main thread (PresMain: the window's pump and the watchdog)" % (m.group(1), wait[:36])); continue
        said = next((DXVK_ROWS[x] for x in dx[:2] if DXVK_ROWS.get(x)), "")
        out.append("  thread %-6s in %-36s DXVK rows: %-36s driver rows above them: %-34s %s%s" % (
            m.group(1), wait[:36], " ".join(dx[:5]) or "none", " ".join(drv[:5]) or "none",
            ("this program's: " + ", ".join(fn) + "; ") if fn else "", said))
    out.append("  and %d other threads with no row of DXVK's and no function of this program's (the driver's own, the system's pool)" % quiet)
    return "\n".join(out)


def kill_pid(pid):
    if pid: subprocess.run(["taskkill", "/F", "/PID", str(pid)], capture_output=True)


def pid_alive(pid):
    r = subprocess.run(["tasklist", "/FI", "PID eq %s" % pid, "/NH", "/FO", "CSV"], capture_output=True, text=True, errors="replace")
    return ('"%s"' % pid) in r.stdout


def freeze_run(rid, name, title, helper_args, fake_args, interval, want_fg, syms, started):
    print("FREEZE RUN %s: %s" % (name, title))
    rd = os.path.join(OUT, "freeze", name)
    os.makedirs(rd)
    conf = os.path.join(rd, "dxvk.conf")
    with open(conf, "w") as f: f.write(FREEZE_CONF % interval)
    env = dict(os.environ)
    env["DXVK_CONFIG_FILE"] = conf; env["DXVK_CONFIG"] = FREEZE_ENV; env["DXVK_LOG_PATH"] = rd
    hlog = os.path.join(rd, "helper-log.txt")
    cmd = [os.path.join(OUT, "fgpipe.exe"), os.path.join(OUT, "full", "dlssiv-helper.exe"), hlog, "--fake-fg=30", "--fake-want-fg=%d" % want_fg,
           "--fake-pres-dxvk=" + os.path.join(OUT, "dxvk", "d3d9.dll"), "--fake-pres-at=%d" % FREEZE_AT] + FREEZE_QUIET + fake_args + helper_args
    t0 = time.time()
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors="replace", cwd=os.path.join(OUT, "full"), env=env)
    started.append(p.pid)
    q = queue.Queue()

    def reader():
        for line in p.stdout: q.put(line.rstrip("\n"))
        q.put(None)
    threading.Thread(target=reader, daemon=True).start()
    lines, hpid, snap, deadline = [], None, "", False
    while True:
        try: line = q.get(timeout=0.25)
        except queue.Empty:
            if time.time() - t0 > FREEZE_DEADLINE: deadline = True; break
            continue
        if line is None: break
        lines.append(line); print("    " + line[:1500])
        m = re.search(r"freeze-helper pid=(\d+)", line)
        if m: hpid = int(m.group(1)); started.append(hpid)
        if "freeze-snapshot-wanted" in line: break
    wanted = any("freeze-snapshot-wanted" in l for l in lines)
    if wanted or deadline:
        # WHERE EVERY THREAD OF THE FAKE CLIENT STANDS, before anything is ended (hangstacks.py takes the last process of that name)
        try:
            r = subprocess.run([sys.executable, os.path.join(ROOT, "tools", "hangstacks.py"), "fgpipe.exe"], capture_output=True, text=True, errors="replace", timeout=60)
            snap = r.stdout
        except subprocess.TimeoutExpired:
            snap = ""
        with open(os.path.join(rd, "hang.txt"), "w") as f: f.write(snap)
        if snap.startswith("pid %d," % p.pid):
            summary = snapshot_summary(snap, syms)
            with open(os.path.join(rd, "hang-summary.txt"), "w") as f: f.write(summary + "\n")
            print("    THE SNAPSHOT (tools\\hangstacks.py, every thread of the fake client as it stood; the whole of it is hang.txt beside this run):")
            for l in summary.split("\n"): print("    " + l[:400])
        else:
            print("    THE SNAPSHOT COULD NOT BE TAKEN of pid %d (%s)" % (p.pid, snap.split("\n")[0][:120] if snap else "no output"))
        kill_pid(p.pid)
    try: p.wait(timeout=30)
    except subprocess.TimeoutExpired: kill_pid(p.pid); p.wait(timeout=30)
    kill_pid(hpid)
    with open(os.path.join(rd, "out.txt"), "w") as f: f.write("\n".join(lines) + "\n")
    res = {"id": rid, "name": name, "title": title, "dir": rd, "deadline": deadline, "took": time.time() - t0, "snap": bool(snap)}
    m = next((l for l in lines if "freeze-result " in l), None)
    res["r"] = dict(t.split("=", 1) for t in m.split("freeze-result ", 1)[1].split()) if m else {}
    # no report at the deadline is a freeze of the whole instrument; a fake client that ENDED without reporting is not a
    # freeze, it is a run that did not happen, and its exit code is said
    res["verdict"] = res["r"].get("verdict") if m else "FROZEN" if deadline else "DIED"
    if not m: print("    THE FAKE CLIENT DID NOT REPORT: %s" % ("this script's deadline of %d s passed: snapshotted and ended" % FREEZE_DEADLINE if deadline else "it ended by itself with exit code 0x%X" % (p.returncode & 0xFFFFFFFF)))
    for l in lines:
        if l.startswith("  PASS  "): passes.append((name, l))
        if l.startswith("  FAIL  "): fails.append((name, l))
    nf = len([l for l in lines if l.startswith("  FAIL  ")])
    proof(m is not None and nf == 0, name, "the fake client reported its measurement and every proof of its own held (%d failed)" % nf)
    d3 = os.path.join(rd, "fgpipe_d3d9.log")
    dlog = open(d3, errors="replace").read() if os.path.exists(d3) else ""
    modes = re.findall(r"Present mode:\s+(VK_PRESENT_MODE_\w+)", dlog)
    images = re.findall(r"Image count:\s+(\d+)", dlog)
    res["mode"] = modes[-1].replace("VK_PRESENT_MODE_", "").replace("_KHR", "") if modes else "NOT IN DXVK'S LOG"
    proof("DXVK: v3.1.1" in dlog and bool(modes) and (modes[-1] == "VK_PRESENT_MODE_FIFO_KHR") == (interval == 1) and images[-1:] == ["3"], name,
          "DXVK's own log beside the run: v3.1.1, present mode %s (%s wanted), %s images" % (res["mode"], "FIFO" if interval == 1 else "not FIFO", images[-1] if images else "?"))
    hl = open(hlog, errors="replace").read() if os.path.exists(hlog) else ""
    lever = next((int(a.split("=", 1)[1]) for a in helper_args if a.startswith("--fghold_q=")), None)
    if lever is not None:
        proof(re.search(r"--fghold_q: this helper sleeps 8 s before the prepare of pair %d " % lever, hl) is not None, name,
              "the helper's own log says its lever held the prepare of pair %d" % lever)
    return res


def freeze_setup():
    """What every freeze run needs: the DXVK that VERSION.txt records, staged beside the runs, and the fake client's own
    functions from its map. None when the file is not that DXVK."""
    md5 = hashlib.md5(open(DXVK, "rb").read()).hexdigest() if os.path.exists(DXVK) else ""
    vt = open(os.path.join(ROOT, "external", "dxvk", "VERSION.txt"), errors="replace").read()
    if not proof(bool(md5) and ("MD5 " + md5) in vt, "freeze", "external\\dxvk\\x32\\d3d9.dll is the DXVK that VERSION.txt records (MD5 %s): the file the game runs as vulkan.dll" % (md5 or "NOT THERE")):
        return None
    os.makedirs(os.path.join(OUT, "dxvk")); os.makedirs(os.path.join(OUT, "freeze"))
    shutil.copy2(DXVK, os.path.join(OUT, "dxvk", "d3d9.dll"))
    return map_functions(os.path.join(OUT, "fgpipe.map"))


def freeze_table(results):
    """The runs' table, printed and kept. The gate's columns are the gate's own counts (IvtGate) for --fake-pres-gate=2,
    the test code's for 1, and "-" with no gate: presents held, every hold together and the longest in ms (its own clock,
    QueryPerformanceCounter), given up, unread (a counter's read that FAILED: 0 in a healthy session), and the presents
    it did not look at (vsync off). Then the fake client's own clock: what a call that did not hold cost in
    microseconds, on average and at most. "no src" is the pairs that found
    the pair before them unanswered at their hand over the whole run: a session at 59 frames a second in which the
    helper kept up has none or a few, and one with dozens was disturbed (something else had the machine)."""
    print("THE FREEZE RUNS (a verdict is a measurement; FROZEN: the presenting thread was still inside one call when the verdict was taken)")
    head = ("run", "verdict", "mode", "before", "during", "after", "longest ms", "frame", "waits", "no src", "stall ms", "gate", "held", "holds ms", "longest hold", "gave up", "unread", "no look", "call us", "call max us", "where the presenting thread stood when the verdict was taken")
    fmt = "%-7s %-10s %-9s %7s %7s %7s  %10s %-6s %6s %6s  %9s  %4s %5s %10s %12s %7s %6s %7s %8s %11s  %s"
    table = [fmt % head]
    where = ["between calls", "the scene's D3D9 calls", "FlushRenderingCommands", "FgxPrepWait", "LockSubmissionQueue", "vkQueueSubmit", "a pipe write", "a copy's lock and submit", "Present", "the frame-end marker", "THE GATE before Present"]
    for x in results:
        r = x["r"]
        st = int(r.get("state", "0"))
        gate = r.get("gate", "0")
        g = (lambda k: r.get(k, "?")) if gate != "0" else (lambda k: "-")
        table.append(fmt % (
            x["name"], x["verdict"][:10], x["mode"], r.get("presents_before", "?"), r.get("presents_during", "?"), r.get("presents_after", "?"), r.get("longest_ms", "?"), r.get("longest_frame", "?"),
            r.get("waits_queued", "?"), r.get("nosource", "?"), r.get("stall_ms", "?"),
            {"0": "none", "1": "1", "2": "own"}.get(gate, "?"), g("gate_held"), g("gate_hold_total_ms"), g("gate_hold_max_ms"), g("gate_gave_up"), g("gate_unread"), g("gate_no_look"),
            r.get("gate_us_mean", "?") if gate == "2" else "-", r.get("gate_us_max", "?") if gate == "2" else "-",
            ("inside %s for %s ms" % (where[st] if st < len(where) else "?", r.get("state_ms", "?"))) if r else "no report"))
    for l in table: print("  " + l)
    with open(os.path.join(OUT, "freeze", "table.txt"), "w") as f: f.write("\n".join(table) + "\n")


def freeze_finish(started):
    """No process this script started is left: each is asked for by its pid, ended if it is there, and asked for again."""
    left = [pid for pid in started if pid_alive(pid)]
    for pid in left: kill_pid(pid)
    left = [pid for pid in started if pid_alive(pid)]
    proof(not left, "freeze", "none of the %d processes this script started is left (%s)" % (len(started), "checked by pid with tasklist" if not left else "STILL THERE: %s" % left))
    names = subprocess.run(["tasklist", "/FO", "CSV", "/NH"], capture_output=True, text=True, errors="replace").stdout.lower()
    print("  (tasklist by name, for the record: %d fgpipe.exe, %d dlssiv-helper.exe running on this machine now; a game's own helper would be counted here and was not touched)" % (names.count('"fgpipe.exe"'), names.count('"dlssiv-helper.exe"')))


def freeze_main():
    want = [x for x in FREEZE.split("=", 1)[1].split(",") if x] if "=" in FREEZE else [r[0] for r in FREEZE_RUNS]
    unknown = [x for x in want if x not in [r[0] for r in FREEZE_RUNS]]
    if unknown: print("FAIL: no freeze run is called %s (they are: %s)" % (", ".join(unknown), ", ".join(r[0] for r in FREEZE_RUNS))); return 1
    syms = freeze_setup()
    if syms is None: print("FAIL"); return 1
    started, results = [], []
    for rid, title, hargs, fargs, interval, want_fg in FREEZE_RUNS:
        if rid not in want: continue
        for k in range(1, FREEZE_TIMES + 1):
            results.append(freeze_run(rid, rid if FREEZE_TIMES == 1 else "%s-%d" % (rid, k), title, hargs, fargs, interval, want_fg, syms, started))
    freeze_table(results)
    freeze_finish(started)
    for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
    print("%d proofs of the instrument passed, %d failed; %d freeze runs; everything of each run is kept under %s" % (len(passes), len(fails), len(results), os.path.join(OUT, "freeze")))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


def freeze_proof_main():
    """THE STANDING PROOF OF THE GATE (the head says what it holds): the main case with the mod's one gate must
    RECOVER with exactly one present held and none given up; the main case without it, once, is said and fails nothing."""
    runs = {r[0]: r for r in FREEZE_RUNS}
    syms = freeze_setup()
    if syms is None: print("FAIL"); return 1
    started, results = [], []
    for rid in ("G2", "main"):
        _, title, hargs, fargs, interval, want_fg = runs[rid]
        results.append(freeze_run(rid, rid, title, hargs, fargs, interval, want_fg, syms, started))
    freeze_table(results)
    g, m = results[0], results[1]
    r = g["r"]
    proof(g["verdict"] == "RECOVERED", "freeze-proof", "THE MAIN CASE WITH THE GATE (the mod's one gate): %s (RECOVERED wanted): %s presents returned after the release, the longest of the run took %s ms"
          % (g["verdict"], r.get("presents_after", "?"), r.get("longest_ms", "?")))
    # (HELD EXACTLY ONCE is a quiet machine's number. When the helper is late with a frame for any other reason, the gate
    # holds that present too, for as long as the helper is late: measured, a few milliseconds each on a machine that was
    # busy with something else. Such a session shows it in its pairs with no source, which is said beside the count.)
    busy = r.get("nosource", "0").isdigit() and int(r.get("nosource", "0")) >= 24
    proof(r.get("gate") == "2" and r.get("gate_held") == "1" and r.get("gate_gave_up") == "0", "freeze-proof",
          "the gate held %s present(s) and gave up on %s (exactly 1 and 0 wanted): its longest hold %s ms, all its holds together %s ms, %s call(s), a counter's read failed %s time(s); "
          "%s of the session's %s pairs found the pair before them unanswered%s"
          % (r.get("gate_held", "?"), r.get("gate_gave_up", "?"), r.get("gate_hold_max_ms", "?"), r.get("gate_hold_total_ms", "?"), r.get("gate_calls", "?"), r.get("gate_unread", "?"),
             r.get("nosource", "?"), r.get("q_written", "?"), ": THE MACHINE WAS BUSY WITH SOMETHING ELSE, and this proof is for a quiet one (run it again)" if busy else ""))
    if m["verdict"] == "FROZEN":
        print("  THE CONTROL (informational): the main case WITHOUT the gate is FROZEN, as on 2026-10-06: on this driver the gate is what lets the run recover")
    elif m["verdict"] == "RECOVERED":
        print("  THE CONTROL (informational): the main case WITHOUT the gate RECOVERED: THE DRIVER NO LONGER SHOWS THE FAULT on this machine. Nothing fails for it; the run above then shows that the gate does no harm, not that it is what saves the run")
    else:
        print("  THE CONTROL (informational): the main case WITHOUT the gate reads %s, which is neither FROZEN nor RECOVERED: read its folder (%s)" % (m["verdict"], m["dir"]))
    freeze_finish(started)
    for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
    print("%d proofs passed, %d failed; the control without the gate: %s; everything of the two runs is kept under %s" % (len(passes), len(fails), m["verdict"], os.path.join(OUT, "freeze")))
    print("PASS" if not fails else "FAIL")
    return 0 if not fails else 1


def fix_counts(s):
    """The helper's two lines for the connection, against what this session's fake client sent: the count the kit reads,
    word for word, and the line of the two fixes' own counts, which is said only when one of them is not zero."""
    sid = s["sid"]
    line = re.search(r"frame generation: this connection: ([^\n]*)", s["log"])
    proof(line is not None and line.group(1).strip() == FIX_COUNTS[sid], sid, "THE HELPER'S OWN COUNT is what the fake client sent: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    more = re.search(r"frame generation: this connection, more: answers the queue refused (\d+); lists the ring refused (\d+)", s["log"])
    got = (int(more.group(1)), int(more.group(2))) if more else None
    want = FIX_MORE[sid]
    proof(got == want, sid, "and its line of the two fixes' counts: %s (wanted: %s)" % (
        ("answers the queue refused %d, lists the ring refused %d" % got) if got else "no such line",
        ("answers the queue refused %d, lists the ring refused %d" % want) if want else "no such line: neither count moved"))


def fix_control(sid, title, helper_args, fake_args, must_fail):
    """A CONTROL: the same run against a helper told to do what it did until 2026-10-10. The fake client must FAIL exactly
    the proofs named (one line each, by a text of theirs) and no other; its lines are not counted as this script's."""
    s = session(sid, title, "full", helper_args, fake_args)
    hit = [any(t in l for l in s["f"]) for t in must_fail]
    ok = s["rc"] != 0 and s["last"] == "FAIL" and len(s["f"]) == len(must_fail) and all(hit)
    proof(ok, sid, "THE CONTROL FAILS AS IT MUST: %d of the %d proofs named for it failed and %d FAIL lines in all (exit code %d): %s" % (
        sum(hit), len(must_fail), len(s["f"]), s["rc"], " | ".join(l.strip()[:260] for l in s["f"]) or "NO FAIL LINE"))
    return s


def fix_sessions():
    """THE HELPER'S TWO FIXES (the head says what R1 to N2c are). Returns how many sessions ran."""
    n = 0
    fk = ["--fake-want-fg=3"]
    if not REL:
        # ---- R1: every kind of pair, its answer made by the CPU
        lev = ["--fg=1", "--fgrefuse_at=2", "--fgrefuse_n=12", "--fgfail_at=3", "--fgraise_at=6"]
        R1 = session("R1", "helper-wire-18: EVERY KIND OF PAIR, ITS ANSWER MADE BY THE CPU, reads \"no frame\" (--fake-fg=12 --fake-fix=1)", "full", lev, fk + ["--fake-fg=12", "--fake-fix=1"]); n += 1
        clean(R1, FULL_RUN + FIX_RUN["R1"])
        fix_counts(R1)
        says(R1, r"frame generation: --fgrefuse_at: the answers of 'G' 2 to 13 of this connection are made as if the queue had refused the signal", "its lever's line")
        says(R1, r"frame generation: pair 34: the queue refused its answer: no frame, from the CPU", "that pair 34's answer was the CPU's, with no frame")
        says(R1, r"--fgcpu_same", "anything of the control's switch", want=False)
        closed_by(R1, "the game", 45)
        fix_control("R1c", "ITS CONTROL (--fgcpu_same=1): the CPU says the queue's own value, as until 2026-10-10", lev + ["--fgcpu_same=1"], fk + ["--fake-fg=12", "--fake-fix=1"],
                    ["A PAIR THAT WAS DISPATCHED (34"]); n += 1
        # ---- R2: an older answer still queued
        lev = ["--fg=1", "--fgrefuse_at=3"]
        R2 = session("R2", "helper-wire-18: NOTHING THE QUEUE STILL OWES MAY LAND BEHIND A CPU SIGNAL (--fake-fg=12 --fake-fix=2)", "full", lev, fk + ["--fake-fg=12", "--fake-fix=2"]); n += 1
        clean(R2, FULL_RUN + FIX_RUN["R2"])
        fix_counts(R2)
        says(R2, r"frame generation: pair 35: the queue refused its answer: not answered \(an older answer is still queued\)", "that pair 35 was not answered while an older answer was queued")
        closed_by(R2, "the game", 37)
        fix_control("R2c", "ITS CONTROL (--fgcpu_same=1): the CPU signals at once, over a queued answer", lev + ["--fgcpu_same=1"], fk + ["--fake-fg=12", "--fake-fix=2"],
                    ["WITH PAIR 34'S ANSWER STILL QUEUED", "once fgIn is signalled pair 34's own answer lands", "no fence the helper signals was ever read lower than before"]); n += 1
        # ---- R3: the device removed for real
        lev = ["--fg=1", "--fgremove_at=2"]
        R3 = session("R3", "helper-wire-18: THE DEVICE REMOVED behind a dispatch's submit, before its answer (--fake-fg=12 --fake-fix=3)", "full", lev, fk + ["--fake-fg=12", "--fake-fix=3"]); n += 1
        clean(R3, FULL_RUN + FIX_RUN["R3"])
        says(R3, r"frame generation: --fgremove_at: the device is REMOVED behind the submit of pair 34, before its answer \(the interface 0x00000000, the device's reason now 0x887A[0-9A-F]{4}\)", "that the device was removed behind pair 34's submit, by its own reading of the device")
        says(R3, r"the D3D12 device was removed \(0x887A[0-9A-F]{4}\) during a dispatch of frame generation's: exiting", "that it asked its device behind that submit, found it removed and ended the session")
        fix_counts(R3)
        proof(len(R3["closing"]) == 1 and R3["closing"][0] == ("this helper", "34", "0"), "R3", "the helper's closing line: %s (by this helper after 34 frames wanted: frame 35 was not served from a removed device)" % (
            ("by %s after %s frames (%s failed)" % R3["closing"][0]) if R3["closing"] else "NOT THERE"))
        fact = next((l.strip() for l in R3["lines"] if "fix-removed " in l), "NO LINE")
        print("    (R3's measurement of this driver and runtime: %s)" % fact)
        R3c = fix_control("R3c", "ITS CONTROL (--fgcpu_same=1): the device is not asked behind the submit, as until 2026-10-10", lev + ["--fgcpu_same=1"], fk + ["--fake-fg=12", "--fake-fix=3"],
                          ["THE HELPER ENDED THE SESSION ITSELF at that 'G'", "the helper ended by itself with the exit code this run is for"]); n += 1
        linec = re.search(r"frame generation: this connection: ([^\n]*)", R3c["log"])
        proof(linec is not None and "dispatched 2, answered with no frame 0" in linec.group(1) and R3c["closing"][:1] == [("the game", "35", "0")], "R3c",
              "and there the helper COUNTED A DISPATCH for the pair whose list never ran and went on to serve frame 35 from the removed device (its count: %s; closed by %s)" % (
                  linec.group(1).strip()[:120] if linec else "NO LINE", ("%s after %s frames" % R3c["closing"][0][:2]) if R3c["closing"] else "NOT THERE"))
        factc = next((l.strip() for l in R3c["lines"] if "fix-removed " in l), "NO LINE")
        print("    (a fact, not a proof: what the game's side read there: %s)" % factc)
    else:
        print("SESSIONS R1 to R3c: NOT RUN on the third build's helper (the queue's refusal and the removal are the development build's levers)")
    # ---- N1: the real hold on a ring of three (both helpers)
    lev = ["--fg=1"] + ([] if REL else ["--fgring=3"])
    N1 = session("N1", "helper-wire-19: THE RING NEVER BLOCKS THE SERVE THREAD: the real hold on a ring of %s (--fake-fg=13 --fake-fix=1)" % ("eight, the product's" if REL else "three"), "full", lev, fk + ["--fake-fg=13", "--fake-fix=1"]); n += 1
    clean(N1, FULL_RUN + FIX_RUN["N1"])
    fix_counts(N1)
    if not REL: says(N1, r"frame generation: --fgring: the ring's length in use is 3 \(asked 3; this build's is 8\)", "that its ring was three long (the product's length until 2026-10-26)")
    if not REL: proof(len(re.findall(r"frame generation: ring: slot \d not retired: no frame for this pair", N1["log"])) == 2, "N1", "the helper said twice that a slot of its ring had not retired (the prepares of pairs 36 and 37)")
    else: proof(len(re.findall(r"frame generation: ring: slot \d not retired", N1["log"])) == 0, "N1", "THE PRODUCT'S RING OF EIGHT: the held dispatch and the three pairs behind it took no slot that had not retired (no ring line; the upscaler's ring of three bounds the frames behind a hold)")
    says(N1, r"did not retire its allocator slot|was not prepared|the dispatch of pair \d+ failed", "a wait for a slot that timed out, or a refused list taken for a failure", want=False)
    closed_by(N1, "the game", 39)
    if not REL:
        N1c = fix_control("N1c", "ITS CONTROL (--fgring_wait=1): a served list waits up to 2 s for its slot, as until 2026-10-10", lev + ["--fgring_wait=1"], fk + ["--fake-fg=13", "--fake-fix=1"],
                          ["THE SERVE THREAD DID NOT WAIT FOR THE RING"]); n += 1
        proof(len(re.findall(r"frame generation: the GPU did not retire its allocator slot \d in 2 s", N1c["log"])) == 2, "N1c", "and there the helper said twice that the GPU did not retire its slot in 2 s: the old wait is what ran")
        # ---- N2: every slot held, the queue going on
        lev = ["--fg=1", "--fgring_hold=18", "--fgring_holdn=6"]
        N2 = session("N2", "helper-wire-19: EVERY SLOT OF THE RING HELD, THE QUEUE GOING ON (--fake-fg=13 --fake-fix=2)", "full", lev, fk + ["--fake-fg=13", "--fake-fix=2"]); n += 1
        clean(N2, FULL_RUN + FIX_RUN["N2"])
        fix_counts(N2)
        says(N2, r"frame generation: --fgring_hold: the served lists 18 to 23 of this connection find every slot of the ring not retired", "its lever's line")
        says(N2, r"did not retire its allocator slot|was not prepared|the dispatch of pair \d+ failed", "a wait for a slot that timed out, or a refused list taken for a failure", want=False)
        closed_by(N2, "the game", 48)
        fact = next((l.strip() for l in N2["lines"] if "fix-ring-held " in l), "NO LINE")
        print("    (N2's numbers: %s)" % fact)
        fix_control("N2c", "ITS CONTROL (--fgring_wait=1): each of the six lists waits 2 s", lev + ["--fgring_wait=1"], fk + ["--fake-fg=13", "--fake-fix=2"],
                    ["each of the six was answered AT ONCE", "and fgPrep came at once for each", "THE FRAME'S OWN ANSWER KEPT ITS LATENCY", "NOTHING WAITED 2 s"]); n += 1
    else:
        print("SESSIONS N1c, N2, N2c: NOT RUN on the third build's helper (the old wait and the hold are the development build's levers)")
    return n


def wire3_sessions(cap):
    """Sessions X1 to X5 (the head says what each is). Returns how many ran."""
    n = 0
    stampArgs = [] if REL else ["--fake-fg-stamp"]
    # ---- X1: wire 3 on the stock arm (the third build's helper since 2026-10-25: on the own arm, the arm of three counts, X5's expectations)
    arm = "three" if REL else "stock"
    X1 = session("X1", ("WIRE 3 ON THE OWN ARM BY DEFAULT (the third build's helper): four images, the answer at each count, a 'g' of wire 2 (--fake-fg=14)" if REL else
                        "WIRE 3 ON THE STOCK ARM: four images, the answer at each count, the stock arm's fallback, a 'g' of wire 2 (--fake-fg=14)"), "full", ["--fg=1"],
                 ["--fake-want-fg=3", "--fake-fg=14"] + stampArgs); n += 1
    if REL:
        m = re.search(r"the helper says a set of this arm makes counts 0x([0-9A-F]+)", "\n".join(X1["lines"]))
        proof(m is not None and int(m.group(1), 16) == 3, "X1", "THE OWN ARM BY DEFAULT: a set with no fourth image makes counts 0 and 1 (0x3; the stock arm's makes 1 alone, 0x2), and the four-image set's pairs of two are in the count below: it says 0x%s" % (m.group(1) if m else "?"))
    proof(X1["caps"] is not None and bool(X1["caps"] & cap["FG"]) and bool(X1["caps"] & cap["FG_WIRE3"]), "X1", "the helper offers 2048 beside 1024 (caps 0x%X)" % (X1["caps"] or 0))
    clean(X1, FULL_RUN + W3_RUN + (0 if REL else 1))
    closed_by(X1, "the game", W3_FRAMES)
    line = re.search(r"frame generation: this connection: ([^\n]*)", X1["log"])
    proof(line is not None and line.group(1).strip() == W3_COUNTS[arm], "X1", "THE HELPER'S OWN COUNT is what the fake client sent on the %s arm: %s" % ("own" if REL else "stock", line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    more = re.search(r"frame generation: this connection, wire 3: ([^\n]*)", X1["log"])
    proof(more is not None and more.group(1).strip() == W3_MORE[arm], "X1", "and its count of wire 3: %s" % (more.group(1).strip() if more else "NO LINE"))
    four = [float(v) for v in re.findall(r"frame generation: created at 3840x2160 \(guides 3840x2160, format 10, scRGB\)[^\n]*, three images [\d.]+ MiB;[^\n]*this process's video memory [\d.]+ -> ([\d.]+) MiB[^\n]*\n[^\n]*frame generation: wire 3: the set's images are PRESENT, GEN, HUDLESS, GEN2 63\.3 MiB;", X1["log"])]
    cyc = four[1:21]
    proof(len(four) == 22 and len(cyc) == 20 and max(cyc) - min(cyc) <= 64.0, "X1",
          "THE HELPER'S OWN LINE NAMES THE FOUR IMAGES with GEN2's size (63.3 MiB at 3840x2160 FP16) for 22 sets, and its video memory after each of the twenty creates is flat: %s" % (
              ("%.1f to %.1f MiB" % (min(cyc), max(cyc))) if cyc else "NO SUCH LINES (%d)" % len(four)))
    if REL:
        says(X1, r"frame generation: wire 3: the set's images are PRESENT, GEN; the counts of generated frames it makes a pair: 0 \(flow-only\), 1\r?\n", "the set with no fourth image, named, and that it makes flow-only pairs and one frame")
        says(X1, r"frame generation: pair 35 asked for 2 generated frame\(s\), which this set does not make", "the count of two refused (the own arm makes it)", want=False)
    else:
        says(X1, r"frame generation: wire 3: the set's images are PRESENT, GEN; the counts of generated frames it makes a pair: 1, on AMD's library, which makes one frame a pair", "the set with no fourth image, named, and why it makes one frame a pair")
        says(X1, r"frame generation: pair 35 asked for 2 generated frame\(s\), which this set does not make", "the count of two refused on the stock arm, by its line")
    says(X1, r"(?i)DUPLICATE: |DRIFT: |RAISED|did not retire its allocator", "a pair out of step, a raise or a ring that refused a list", want=False)
    # ---- X2: the same helper, the fake client speaking wire 2
    X2 = session("X2", "WIRE 3'S HELPER, THE FAKE CLIENT SPEAKING WIRE 2 (--fake-wire=2 --fake-fg=14)", "full", ["--fg=1"], ["--fake-want-fg=3", "--fake-fg=14", "--fake-wire=2"]); n += 1
    clean(X2, FULL_RUN + W3_RUN_O)
    closed_by(X2, "the game", W3_FRAMES_O)
    line = re.search(r"frame generation: this connection: ([^\n]*)", X2["log"])
    proof(line is not None and line.group(1).strip() == W3_COUNTS["wire2"], "X2", "THE HELPER'S OWN COUNT: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    says(X2, r"frame generation: wire 3|this connection, wire 3", "anything of wire 3 (the client spoke wire 2)", want=False)
    says(X2, r"frame generation: the game asks its sets at wire 2 \(its 'g' has no FEED_FG_BUILD_WIRE3\): one generated frame a pair", "that the game asks at wire 2, in its one line")
    # ---- X3: an old client against this helper
    if OLD_CLIENT:

        shutil.copy2(OLD_CLIENT, os.path.join(OUT, "fgpipe-old.exe"))
        log = os.path.join(OUT, "helper-X3.log")
        print("SESSION X3: AN OLD CLIENT (%s) AGAINST THIS HELPER: session A's whole run" % OLD_CLIENT)
        g = subprocess.run([os.path.join(OUT, "fgpipe-old.exe"), os.path.join(OUT, "full", "dlssiv-helper.exe"), log, "--fake-want-fg=3", "--fake-fg=1", "--fg=1"] + stampArgs,
                           capture_output=True, text=True, errors="replace", cwd=os.path.join(OUT, "full"), timeout=240); n += 1
        lines = g.stdout.splitlines(); hl = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
        p = [l for l in lines if l.startswith("  PASS  ")]; f = [l for l in lines if l.startswith("  FAIL  ")]
        for l in f: print("    " + l)
        # An old client asks a transport build. The shipped helper has none since strip item 11 (2026-10-31), so on --rel exactly
        # the five pixel proofs of that copy fail, and every one of the wire's stands.
        COPY_ONLY = ("frame 1: Output's left half", "frame 1: Output's right half", "THE CONTROL:", "and once the in-fence is signalled", "thirty more frames")
        if REL:
            proof(g.returncode == 1 and lines and lines[-1].strip() == "FAIL" and len(f) == len(COPY_ONLY) and all(any(k in l for l in f) for k in COPY_ONLY) and len(p) >= FULL_RUN + FG_RUN - len(COPY_ONLY)
                  # (those lines also hold the fences' part, and it must stand: the in-fence held, frame 2 answered, all thirty answered)
                  and any("THE CONTROL:" in l and "did not move (it reads 1)" in l for l in f) and not any("(-1 of 4096" in l for l in f) and any("(30 of 30)" in l for l in f), "X3",
                  "the old client's own verdict: %d proofs passed, %d failed, exit code %d: only the five of the transport copy this helper does not have" % (len(p), len(f), g.returncode))
        else:
            proof(g.returncode == 0 and lines and lines[-1].strip() == "PASS" and not f and len(p) >= FULL_RUN + FG_RUN + 2, "X3",
                  "the old client's own verdict: %d proofs passed, %d failed, exit code %d" % (len(p), len(f), g.returncode))
        line = re.search(r"frame generation: this connection: ([^\n]*)", hl)
        proof(line is not None and line.group(1).strip() == FG_COUNTS, "X3", "THE HELPER'S OWN COUNT is session A's: %s" % (line.group(1).strip() if line else "NO LINE"))
        proof(re.search(r"frame generation: wire 3|this connection, wire 3", hl) is None, "X3", "the helper's log says nothing of wire 3 (the old client asked nothing of it)")
        proof(len(re.findall(r"frame generation: the game asks its sets at wire 2", hl)) == 1, "X3", "and it says ONCE that the game asks its sets at wire 2")
    else:
        print("SESSION X3: NOT RUN (no --old-client=<fgpipe.exe built at wire 2>)")
    # ---- X4: this client against an old helper
    if OLD_HELPER:
        d = os.path.join(OUT, "oldhelper")
        if not os.path.isdir(d):
            shutil.copytree(os.path.join(OUT, "full"), d)
            shutil.copy2(OLD_HELPER, os.path.join(d, "dlssiv-helper.exe"))
            if REL: ffx_pin.copy_pinned(ffx_pin.FRAMEGEN, d)   # (the old helper runs on AMD's provider; the third build's folder no longer has it)
        X4 = session("X4", "THIS CLIENT AGAINST AN OLD HELPER (%s): session A's whole run" % OLD_HELPER, "oldhelper", ["--fg=1"], ["--fake-want-fg=3", "--fake-fg=1"] + stampArgs); n += 1
        proof(X4["caps"] is not None and bool(X4["caps"] & cap["FG"]) and not (X4["caps"] & cap["FG_WIRE3"]), "X4", "the old helper offers 1024 without 2048 (caps 0x%X)" % (X4["caps"] or 0))
        clean(X4, FULL_RUN + FG_RUN + (0 if REL else 2))
        line = re.search(r"frame generation: this connection: ([^\n]*)", X4["log"])
        proof(line is not None and line.group(1).strip() == FG_COUNTS, "X4", "THE OLD HELPER'S OWN COUNT is session A's: %s" % (line.group(1).strip() if line else "NO LINE"))
        X4b = session("X4b", "THIS CLIENT'S WIRE 3 MODE AGAINST THE OLD HELPER: it must speak wire 2", "oldhelper", ["--fg=1"], ["--fake-want-fg=3", "--fake-fg=14"]); n += 1
        clean(X4b, FULL_RUN + W3_RUN_O)
        line = re.search(r"frame generation: this connection: ([^\n]*)", X4b["log"])
        proof(line is not None and line.group(1).strip() == W3_COUNTS["wire2"], "X4b", "THE OLD HELPER'S OWN COUNT: %s" % (line.group(1).strip() if line else "NO LINE"))
    else:
        print("SESSION X4: NOT RUN (no --old-helper=<dlssiv-helper.exe built at wire 2>)")
    # ---- X5: the own arm
    if OWN_LIB:
        if REL: print("SESSION X5: NOT RUN on the third build's helper (the own arm is the development build's)")
        else:
            d = os.path.join(OUT, "own")
            if not os.path.isdir(d):
                shutil.copytree(os.path.join(OUT, "full"), d)
                shutil.copy2(OWN_LIB, os.path.join(d, os.path.basename(OWN_LIB)))
            X5 = session("X5", "WIRE 3 ON THE OWN ARM (%s): the three counts" % OWN_LIB, "own", ["--fg=1", "--fgown=%s" % os.path.basename(OWN_LIB)], ["--fake-want-fg=3", "--fake-fg=14", "--fake-fg-stamp"]); n += 1
            m = re.search(r"the helper says a set of this arm makes counts 0x([0-9A-F]+)", "\n".join(X5["lines"]))
            proof(m is not None and int(m.group(1), 16) == 7, "X5", "THE OWN ARM MAKES COUNTS 0, 1 AND 2 (0x7): it says 0x%s%s" % (m.group(1) if m else "?", "" if (m and int(m.group(1), 16) == 7) else
                  " (a library of interface 2, or this helper built against interface 2's header: the STAND-IN, which cannot pass here)"))
            clean(X5, FULL_RUN + W3_RUN + 1)
            line = re.search(r"frame generation: this connection: ([^\n]*)", X5["log"])
            proof(line is not None and line.group(1).strip() == W3_COUNTS["three"], "X5", "THE HELPER'S OWN COUNT on an arm of three counts: %s" % (line.group(1).strip() if line else "NO LINE"))
            more = re.search(r"frame generation: this connection, wire 3: ([^\n]*)", X5["log"])
            proof(more is not None and more.group(1).strip() == W3_MORE["three"], "X5", "and its count of wire 3: %s" % (more.group(1).strip() if more else "NO LINE"))
    else:
        print("SESSION X5: NOT RUN (no --own-lib=<dlssiv-fg.dll>)")
    return n


def twin_sessions(cap):
    """Session X6 (round 5, H): the second input set on the stock arm (fgpipe_wire4.h says what each proof is). Returns how many ran."""
    stampArgs = [] if REL else ["--fake-fg-stamp"]
    X6 = session("X6", "THE SECOND INPUT SET ON THE STOCK ARM: pairs by turns, each set's read by pixels with its control, the sets never mixing (--fake-fg=15)", "full", ["--fg=1"],
                 ["--fake-want-fg=3", "--fake-fg=15"] + stampArgs)
    proof(X6["caps"] is not None and bool(X6["caps"] & cap["FG"]) and bool(X6["caps"] & cap["FG_WIRE3"]) and bool(X6["caps"] & 4096), "X6",
          "the helper offers 4096 beside 1024 and 2048 (caps 0x%X)" % (X6["caps"] or 0))
    clean(X6, FULL_RUN + TW_RUN + (0 if REL else 1))
    closed_by(X6, "the game", TW_FRAMES)
    line = re.search(r"frame generation: this connection: ([^\n]*)", X6["log"])
    proof(line is not None and line.group(1).strip() == TW_COUNTS, "X6", "THE HELPER'S OWN COUNT is what the fake client sent: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    if not REL:
        more = re.search(r"frame generation: this connection, the second input set: ([^\n]*)", X6["log"])
        proof(more is not None and more.group(1).strip() == TW_SET, "X6", "and its count of the second input set: %s" % (more.group(1).strip() if more else "NO LINE"))
    says(X6, r"frame generation: wire 3: the set's images are PRESENT, GEN, HUDLESS, the second input set PRESENT2, HUDLESS2 and both sets' guides \d+\.\d MiB;", "the set with both sets and their guides (A1), named, with their size")
    says(X6, r"frame generation: wire 3: the set's images are PRESENT, GEN, the second input set PRESENT2 and both sets' guides \d+\.\d MiB;", "the set with the second and no HUD-less image, named")
    says(X6, r"(?i)DUPLICATE: |DRIFT: |RAISED|did not retire its allocator", "a pair out of step, a raise or a ring that refused a list", want=False)
    return 1


def wine_sessions(cap):
    """Sessions WF1 to WF4 (round 7, L): under Wine, the game's own fences (fgpipe_wine.h says what each proof is). Returns how many ran."""
    titles = {1: "UNDER WINE: THE GAME'S OWN FENCES CONNECT: the probe both ways, pairs by pixels, a second set on the same three",
              2: "UNDER WINE: THE HELPER SEES NOTHING (a silent in-fence planted): refused, said once, nothing queued, the upscaler goes on",
              3: "UNDER WINE: THE GAME SEES NOTHING (silent fgPrep and fgOut planted): the set given back, said once, and the control",
              4: "UNDER WINE ON THE CPU BRIDGE: refused before any 'g', said once, the upscaler goes on"}
    for k in (1, 2, 3, 4):
        sid = "WF%d" % k
        L = session(sid, titles[k] + " (--fake-fg=16 --fake-wine=%d)" % k, "full", ["--fg=1"], ["--fake-want-fg=3", "--fake-fg=16", "--fake-wine=%d" % k])
        clean(L, FULL_RUN + WN_RUN[k])
        closed_by(L, "the game", WN_FRAMES[k])
        says(L, r"frame generation: the game's test signal on its in-fence was not seen within 1500 ms", "that it did not see the game's test signal (the planted silent in-fence)", want=(k == 2))
        says(L, r"frame generation: the game's three fences could not be opened", "that the game's three fences could not be opened", want=False)
        says(L, r"frame generation: created at ", "a set created", want=(k in (1, 3)))
        if k == 3: says(L, r"frame generation: the set is released at the game's word", "the set the game gave back, released at its word")
        says(L, r"(?i)DUPLICATE: |DRIFT: |RAISED|did not retire its allocator", "a pair out of step, a raise or a ring that refused a list", want=False)
    return 4


def own_sessions(cap, ref=None):
    """Sessions O1 to O5 (2026-10-25; the head says what each is), on the third build's helper. ref: a session whose upscaler
    bits the others must say (O1's when none is given). Returns how many ran."""
    size, sha = own_record()
    UP = cap["FG_PROBED"] - 1
    NOLIB = cap.get("FG_NOLIB")
    if NOLIB is None: raise SystemExit("FAIL: feed_ipc.h has no FEED_CAP_FG_NOLIB in the form this script reads")
    stage("ownonly", (), own="lib")
    stage("ownbyte", (ffx_pin.LOADER, ffx_pin.UPSCALER), own="byte")
    stage("ownlong", (ffx_pin.LOADER, ffx_pin.UPSCALER), own="long")
    stage("ownamd", (ffx_pin.LOADER, ffx_pin.UPSCALER, ffx_pin.FRAMEGEN))
    if not os.path.isdir(os.path.join(OUT, "nofg")): stage("nofg", (ffx_pin.LOADER, ffx_pin.UPSCALER))
    # ---- O1: the default, with no file of AMD's at all
    O1 = session("O1", "THE DEFAULT: frame generation on dlssiv-fg.dll with no argument and NO FILE OF AMD's beside the helper: session A's whole run", "ownonly", ["--fg=1"],
                 ["--fake-want-fg=3", "--fake-fg=1"])
    clean(O1, FULL_RUN + FG_RUN)
    closed_by(O1, "the game", FG_FRAMES)
    proof(O1["caps"] is not None and bool(O1["caps"] & cap["FG"]) and not (O1["caps"] & NOLIB), "O1", "the handshake offers frame generation (1024) and not FEED_CAP_FG_NOLIB (caps 0x%X)" % (O1["caps"] or 0))
    says(O1, r"frame generation: fgown [^\n]*\\ownonly\\dlssiv-fg\.dll, %d bytes, sha256 %s: loaded direct" % (size, sha), "the library's load, by its full path beside the helper, with the record's size and hash")
    says(O1, r"frame generation: available, provider \"3\.\d+\.\d+\" id 0x[0-9A-F]+ by the mod's own interface 3; the upscaler's library is not mapped in this process", "the probe's line by the mod's own interface, with no library of AMD's in the process")
    line = re.search(r"frame generation: this connection: ([^\n]*)", O1["log"])
    proof(line is not None and line.group(1).strip() == FG_COUNTS, "O1", "THE HELPER'S OWN COUNT is session A's: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    says(O1, r"(?i)RAISED|did not retire its allocator|amd_fidelityfx_framegeneration", "a raise, a ring that refused a list, or AMD's frame generation file (session A's run sends its one duplicate and its one drift on purpose)", want=False)
    ref = ref or O1
    # ---- O2 to O5: refused, one line, the upscaler's frames go on
    for sid, folder, title, why in (("O2", "nofg", "THE LIBRARY MISSING (AMD's loader and upscaler beside the helper)", r"\\nofg\\dlssiv-fg\.dll: not read \(2\): off"),
                                    ("O3", "ownbyte", "ONE BYTE CHANGED, THE SAME SIZE", r"\\ownbyte\\dlssiv-fg\.dll, %d bytes, sha256 [0-9a-f]{64}: not the expected sha256 %s: off" % (size, sha)),
                                    ("O4", "ownlong", "ONE BYTE MORE", r"\\ownlong\\dlssiv-fg\.dll, %d bytes, sha256 [0-9a-f]{64}: not the expected %d bytes: off" % (size + 1, size)),
                                    ("O5", "ownamd", "AMD's FRAME GENERATION PROVIDER BESIDE IT, THE OWN LIBRARY MISSING", r"\\ownamd\\dlssiv-fg\.dll: not read \(2\): off")):
        S = session(sid, "%s: frame generation off with one line, FEED_CAP_FG_NOLIB, and the upscaler's whole run of frames" % title, folder, ["--fg=1"], ["--fake-want-fg=1"])
        clean(S, FULL_RUN)
        closed_by(S, "the game", 32)
        proof(S["caps"] is not None and bool(S["caps"] & cap["FG_PROBED"]) and not (S["caps"] & cap["FG"]) and bool(S["caps"] & NOLIB), sid,
              "the handshake says asked (256), not offered (no 1024), and FEED_CAP_FG_NOLIB (%d), the warnings line's cause (caps 0x%X)" % (NOLIB, S["caps"] or 0))
        proof(S["caps"] is not None and ref["caps"] is not None and (S["caps"] & UP) == (ref["caps"] & UP), sid,
              "what it says of the upscalers is what %s said (0x%X against 0x%X): the refusal was frame generation's alone" % (ref["sid"], (S["caps"] or 0) & UP, (ref["caps"] or 0) & UP))
        says(S, r"frame generation: fgown [^\n]*" + why, "THE ONE LINE, naming the file and why it was refused")
        n1 = len(re.findall(r"frame generation: (fgown|not offered|available)", S["log"]))
        proof(n1 == 1, sid, "and it is the only line of the helper's that says whether frame generation is there (%d such lines)" % n1)
        says(S, r"(?i)RAISED|amd_fidelityfx_framegeneration", "a raise, or AMD's frame generation file named", want=False)
    return 5


def main():
    if not os.path.exists(HELPER):
        print("FAIL: %s is not built (run build-host first)" % HELPER); return 1
    if (FREEZE or FREEZE_PROOF) and REL:
        print("FAIL: the freeze runs are the development helper's (the main case's hold is its lever, --fghold_q)"); return 1
    if FREEZE and FREEZE_PROOF:
        print("FAIL: --freeze and --freeze-proof are two things: the runs as measurements, or the gate's standing proof"); return 1
    if FLAVOUR not in open(HELPER, "rb").read():
        print("FAIL: host\\dlssiv-helper.exe is not the %s helper this run is for" % ("third build's (build-host.ps1 -Release -Fg)" if REL else "development (build-host.ps1)")); return 1
    bad = ffx_pin.cross_checks()
    if bad:
        for w in bad: print("  " + w)
        print("FAIL: the pins of AMD's libraries disagree with the packager's or with VERSION.txt"); return 1
    cap = caps_bits()
    UP = cap["FG_PROBED"] - 1                       # every bit below frame generation's two: what the helper says of the upscalers
    shutil.rmtree(OUT, ignore_errors=True)
    os.makedirs(OUT)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    # (/MAP since 2026-10-06: the freeze runs name this program's own functions in a snapshot of its threads from it)
    cl = ('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W3 /std:c++17 /DWIN32 /I"%s" /I"%s" /I"%s" "%s" /Fe:fgpipe.exe /Fo:fgpipe.obj /link /INCREMENTAL:NO /MAP:fgpipe.map'
          % (vcvars, os.path.join(ROOT, "external", "vulkan"), os.path.join(ROOT, "source", "transport"), os.path.join(ROOT, "source", "fg"), os.path.join(HERE, "fgpipe.cpp")))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0 or "warning C" in r.stdout:
        print(r.stdout[-3000:]); print("FAIL: the fake client did not compile cleanly"); return 1
    if OWN and not REL:
        print("FAIL: --own is the third build's (--rel): the development helper runs the own library only when --fgown asks"); return 1
    # (2026-10-25) THE THIRD BUILD'S HELPER RUNS ON THE MOD'S OWN LIBRARY: its folders carry that, and NOT AMD's frame generation provider
    three = (ffx_pin.LOADER, ffx_pin.UPSCALER) if REL else (ffx_pin.LOADER, ffx_pin.UPSCALER, ffx_pin.FRAMEGEN)
    if REL: own_record()
    if OWN:                         # sessions O1 to O5 alone (the head says what they are)
        print("the third build's (release -Fg) helper %s; the mod's own library is the record's; sessions O1 to O5 run under %s" % (HELPER, OUT))
        ns = own_sessions(caps_bits())
        for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
        print("%d proofs passed, %d failed; %d sessions" % (len(passes), len(fails), ns))
        print("PASS" if not fails else "FAIL (the folder is kept: %s)" % OUT)
        return 0 if not fails else 1
    stage("full", three, own="lib" if REL else None)
    if not (FREEZE or FREEZE_PROOF or FIXES or TWIN):   # X3 and X4's old halves, pinned by commit (the head says how; not the second input set's session)
        global OLD_CLIENT, OLD_HELPER
        bc, bh = base_binaries(vcvars)
        if bc is None: print("FAIL: %s" % bh); return 1
        OLD_CLIENT, OLD_HELPER = OLD_CLIENT or bc, OLD_HELPER or bh
    if FREEZE or FREEZE_PROOF:      # the freeze runs, asked for by name, and nothing else (the head says why they are never part of the sessions)
        print("the development helper; AMD's three libraries are the pinned files; the freeze runs are kept under %s" % os.path.join(OUT, "freeze"))
        return freeze_proof_main() if FREEZE_PROOF else freeze_main()
    if FIXES:                       # the two fixes' sessions alone (the head says what they are)
        print("the %s helper; AMD's three libraries are the pinned files; the two fixes' sessions run under %s" % ("third build's (release -Fg)" if REL else "development", OUT))
        ns = fix_sessions()
        for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
        print("%d proofs passed (the fixes' floor is %d), %d failed; %d sessions" % (len(passes), MIN_FIXES, len(fails), ns))
        ok = not fails and len(passes) >= MIN_FIXES
        print("PASS" if ok else "FAIL (the folder is kept: %s)" % OUT)
        return 0 if ok else 1
    if TWIN:                        # the second input set's session alone (round 5, H; TW_RUN's comment says what it is)
        print("the %s helper; AMD's three libraries are the pinned files; the second input set's session runs under %s" % ("third build's (release -Fg)" if REL else "development", OUT))
        ns = twin_sessions(cap)
        for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
        print("%d proofs passed, %d failed; %d sessions" % (len(passes), len(fails), ns))
        print("PASS" if not fails else "FAIL (the folder is kept: %s)" % OUT)
        return 0 if not fails else 1
    if WINE:                        # under Wine, sessions WF1 to WF4 alone (round 7, L; the head says what they are)
        print("the %s helper; AMD's three libraries are the pinned files; the Wine sessions run under %s" % ("third build's (release -Fg)" if REL else "development", OUT))
        ns = wine_sessions(cap)
        for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
        print("%d proofs passed, %d failed; %d sessions" % (len(passes), len(fails), ns))
        print("PASS" if not fails else "FAIL (the folder is kept: %s)" % OUT)
        return 0 if not fails else 1
    if WIRE3:                       # wire 3's sessions alone (the head says what they are)
        print("the %s helper; AMD's three libraries are the pinned files; wire 3's sessions run under %s" % ("third build's (release -Fg)" if REL else "development", OUT))
        ns = wire3_sessions(cap)
        for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
        print("%d proofs passed, %d failed; %d sessions" % (len(passes), len(fails), ns))
        print("PASS" if not fails else "FAIL (the folder is kept: %s)" % OUT)
        return 0 if not fails else 1
    stage("nofg", (ffx_pin.LOADER, ffx_pin.UPSCALER))
    stage("noffx", (), own="lib" if REL else None)
    have_agility = os.path.exists(AGILITY)
    if have_agility: stage("agility", three, agility=True, own="lib" if REL else None)
    print("the %s helper; AMD's three libraries are the pinned files; sessions run under %s" % ("third build's (release -Fg)" if REL else "development", OUT))
    n_sessions = 0

    # ---- B first: what this helper says of the upscalers when frame generation is not asked (DLSS) -----------------------
    B = session("B", "DLSS, frame generation not asked; then the tag 'g' alone", "full", [], ["--fake-want-fg=0", "--fake-send-tag=g"]); n_sessions += 1
    clean(B, HELLO_RUN)
    proof(B["caps"] is not None and bool(B["caps"] & cap["DLSS"]), "B", "this is a DLSS session (the instrument's own condition: caps 0x%X has DLSS available)" % (B["caps"] or 0))
    says(B, r"unknown tag 0x67: stopping", "\"unknown tag 0x67: stopping\", as a helper without this code does")
    says(B, r"(?i)frame generation", "anything of frame generation (it was not asked)", want=False)
    closed_by(B, "this helper", 0)

    # ---- A: the whole run, asked ------------------------------------------------------------------------------------------
    A = session("A", "DLSS, frame generation asked: the whole run, then frame generation's wire", "full", ["--fg=1"],
                ["--fake-want-fg=3", "--fake-fg=1"] + ([] if REL else ["--fake-fg-stamp"])); n_sessions += 1
    clean(A, FULL_RUN + FG_RUN + (0 if REL else 2))
    control = [l for l in A["p"] if "THE CONTROL:" in l and "as it must" in l]
    proof(len(control) == 1, "A", "the control's line is there and passed (a frame whose in-fence is withheld is not answered)")
    held = [l for l in A["p"] if ("THE HELD 'Q':" in l or "THE HELD 'G':" in l) and "as it must" in l]
    proof(len(held) == 2, "A", "the two held cases are there and passed (fgPrep withheld behind the frame's in-fence, fgOut and the next frame behind fgIn)")
    closed_by(A, "the game", FG_FRAMES)
    line = re.search(r"frame generation: this connection: ([^\n]*)", A["log"])
    proof(line is not None and line.group(1).strip() == FG_COUNTS, "A", "THE HELPER'S OWN COUNT is what the fake client sent: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    made = [float(x) for x in re.findall(r"frame generation: created at 3840x2160 \(guides 3840x2160, format 10, scRGB\)[^\n]*this process's video memory [\d.]+ -> ([\d.]+) MiB", A["log"])]
    cyc = made[1:21]     # the first set, then the twenty cycles (the second format's set is format 87 and is not in this list)
    proof(len(made) == 23 and len(cyc) == 20 and max(cyc) - min(cyc) <= 64.0, "A",
          "the helper's video memory after each of the twenty creates is flat: %s" % (("%.1f to %.1f MiB over %d creates (%d sets of this format in all)" % (min(cyc), max(cyc), len(cyc), len(made))) if cyc else "NO CREATE LINES"))
    says(A, r"frame generation: 'g' refused: the presented format 2 is not one of the five", "the refused 'g', with its reason")
    if REL: says(A, r"frame generation: available, provider \"3\.\d+\.\d+\" id 0x[0-9A-F]+ by the mod's own interface 3", "\"frame generation: available\" by the mod's own library, with no argument")
    else:   says(A, r"frame generation: available, provider \"3\.\d+\.\d+\" id 0x[0-9A-F]+ taken of \d+ listed", "\"frame generation: available\" with the provider it took")
    proof(A["caps"] is not None and B["caps"] is not None and (A["caps"] & UP) == (B["caps"] & UP), "A",
          "what it says of the upscalers is what B said (0x%X against 0x%X): asking for frame generation did not make it claim FSR" % ((A["caps"] or 0) & UP, (B["caps"] or 0) & UP))
    m = re.search(r"asked in ([\d.]+) ms; the upscaler's library is (mapped|not mapped)", A["log"])
    print("    (a fact, not a proof: on this DLSS session the probe took %s ms and the upscaler's library is %s in the helper)" % (m.group(1), m.group(2)) if m else "    (the probe's line has no time)")

    # ---- C and D: FSR ---------------------------------------------------------------------------------------------------
    C = session("C", "FSR, frame generation not asked", "full", ["--backend=1"], ["--fake-want-fg=0", "--fake-hello-only"]); n_sessions += 1
    clean(C, HELLO_RUN)
    proof(C["caps"] is not None and bool(C["caps"] & cap["FSR"]), "C", "this is an FSR session (the instrument's own condition: caps 0x%X has FSR available)" % (C["caps"] or 0))
    D = session("D", "FSR, frame generation asked", "full", ["--backend=1", "--fg=1"], ["--fake-want-fg=3", "--fake-hello-only"]); n_sessions += 1
    clean(D, HELLO_RUN)
    proof(D["caps"] is not None and C["caps"] is not None and (D["caps"] & UP) == (C["caps"] & UP), "D", "what it says of the upscalers is what C said (0x%X against 0x%X)" % ((D["caps"] or 0) & UP, (C["caps"] or 0) & UP))
    says(D, r"frame generation: available, provider .*the upscaler's library is mapped", "\"frame generation: available\", through the loader FSR holds")

    # ---- E: the library moved away, then 'g' ------------------------------------------------------------------------------
    E = session("E", "DLSS, asked, the frame generation library moved away; then the tag 'g' alone", "nofg", ["--fg=1"], ["--fake-want-fg=1", "--fake-send-tag=g"]); n_sessions += 1
    clean(E, HELLO_RUN)
    if REL: says(E, r"frame generation: fgown [^\n]*\\nofg\\dlssiv-fg\.dll: not read \(2\): off", "why frame generation is not offered, naming the mod's own library")
    else:   says(E, r"frame generation: not offered, because the library (answered the version query with|lists no frame generation provider)", "why frame generation is not offered, naming the library")
    proof(E["caps"] is not None and bool(E["caps"] & cap["FG_NOLIB"]) == REL, "E", "FEED_CAP_FG_NOLIB %s (caps 0x%X)" % ("said: the own library is missing" if REL else "NOT said: the development helper's AMD arm", E["caps"] or 0))
    says(E, r"the game sent its message 'g', and this helper did not offer frame generation at the handshake: stopping", "the door's own reason for ending the session")
    says(E, r"unknown tag", "\"unknown tag\" (the door took the message, the loop's last line did not)", want=False)
    closed_by(E, "this helper", 0)
    proof(E["caps"] is not None and B["caps"] is not None and (E["caps"] & UP) == (B["caps"] & UP), "E", "what it says of the upscalers is what B said (0x%X against 0x%X)" % ((E["caps"] or 0) & UP, (B["caps"] or 0) & UP))

    # ---- F: no FidelityFX file at all -------------------------------------------------------------------------------------
    F = session("F", "DLSS, asked, no FidelityFX file beside the helper" + (" (the mod's own library beside it: frame generation needs no file of AMD's)" if REL else ""), "noffx", ["--fg=1"],
                ["--fake-want-fg=%d" % (3 if REL else 1), "--fake-hello-only"]); n_sessions += 1
    clean(F, HELLO_RUN)
    if REL: says(F, r"frame generation: available, provider \"3\.\d+\.\d+\" id 0x[0-9A-F]+ by the mod's own interface 3; the upscaler's library is not mapped", "\"frame generation: available\" with no file of AMD's in the folder")
    else:   says(F, r"frame generation: not offered, because .*amd_fidelityfx_loader_dx12\.dll would not load \(\d+\)", "that the loader would not load, by its path")
    proof(F["caps"] is not None and B["caps"] is not None and (F["caps"] & UP) == (B["caps"] & UP), "F", "what it says of the upscalers is what B said (0x%X against 0x%X)" % ((F["caps"] or 0) & UP, (B["caps"] or 0) & UP))

    # ---- G: offered, and a tag that is nobody's -----------------------------------------------------------------------------
    G = session("G", "DLSS, asked and available; then a tag that is nobody's", "full", ["--fg=1"], ["--fake-want-fg=3", "--fake-send-tag=Z"]); n_sessions += 1
    clean(G, HELLO_RUN)
    says(G, r"unknown tag 0x5A: stopping", "\"unknown tag 0x5A: stopping\": the door answered \"not mine\" and the loop's own line followed")
    closed_by(G, "this helper", 0)

    # ---- H: the raise lever (development) ---------------------------------------------------------------------------------
    if not REL:
        H = session("H", "THE RAISE LEVER: the version query raises (--fgraise=2); the whole run", "full", ["--fg=1", "--fgraise=2"], ["--fake-want-fg=1"]); n_sessions += 1
        clean(H, FULL_RUN)
        says(H, r"frame generation: not offered, because AMD's library RAISED exception 0xE0F60002", "the raise, with its code, as the reason")
        closed_by(H, "the game", 32)
        says(H, r"a library raised earlier: this helper ends without unloading it", "that it ends without unloading a library that raised")
        proof(H["caps"] is not None and B["caps"] is not None and (H["caps"] & UP) == (B["caps"] & UP), "H",
              "what it says of the upscalers is what B said (0x%X against 0x%X): the raise was frame generation's alone" % ((H["caps"] or 0) & UP, (B["caps"] or 0) & UP))
        # ---- L2, L3, L4: the levers of stage 6 ---------------------------------------------------------------------------
        L2 = session("L2", "THE RAISE AT A DISPATCH (--fgraise_at=2)", "full", ["--fg=1", "--fgraise_at=2"], ["--fake-want-fg=3", "--fake-fg=2"]); n_sessions += 1
        clean(L2, FULL_RUN + 6)
        says(L2, r"frame generation: AMD's library RAISED exception 0xE0F60001[^\n]* dispatching pair 34", "the raise at pair 34's dispatch, with its code")
        says(L2, r"a library raised earlier: this helper ends without unloading it", "that it ends without unloading a library that raised")
        closed_by(L2, "the game", 36)
        L3 = session("L3", "THREE FAILED DISPATCHES (--fgfail_at=2)", "full", ["--fg=1", "--fgfail_at=2"], ["--fake-want-fg=3", "--fake-fg=3"]); n_sessions += 1
        clean(L3, FULL_RUN + 5)
        says(L3, r"the dispatch of pair 36 failed \([^\n]*PLANTED by --fgfail_at\), 3 in a row: frame generation is over for this set", "the third failure in a row, and what follows from it")
        closed_by(L3, "the game", 38)
        L4 = session("L4", "THE VIDEO MEMORY RULE, FORCED (--fgvram_refuse=1)", "full", ["--fg=1", "--fgvram_refuse=1"], ["--fake-want-fg=3", "--fake-fg=4"]); n_sessions += 1
        clean(L4, FULL_RUN + 2)
        says(L4, r"frame generation: 'g' refused by the video memory rule \(FORCED by --fgvram_refuse\): the set needs [\d.]+ MiB \(three images [\d.]+, the context [\d.]+, a margin [\d.]+\) and this process may still take [\d.]+ MiB",
             "the refusal with its numbers")
        closed_by(L4, "the game", 33)
        L5 = session("L5", "LATE: the helper sleeps 3 s before its ack (--fgslow_g=3000), the client's lines give it 1 s", "full", ["--fg=1", "--fgslow_g=3000"], ["--fake-want-fg=3", "--fake-fg=5"]); n_sessions += 1
        clean(L5, FULL_RUN + 1)
        says(L5, r"frame generation: --fgslow_g: this helper sleeps 3000 ms before its ack", "the lever's line (the set was built, and the ack held back)")
        closed_by(L5, "the game", 32)
        L6 = session("L6", "A BROKEN PIPE: the helper ends on 'g' (--fgdie_g=1)", "full", ["--fg=1", "--fgdie_g=1"], ["--fake-want-fg=3", "--fake-fg=6"]); n_sessions += 1
        clean(L6, FULL_RUN + 1)
        says(L6, r"frame generation: --fgdie_g: this helper ends on 'g' with its ack unwritten", "the lever's line")
        proof(not L6["closing"], "L6", "the helper wrote no closing line (it was ended, it did not end)")
        # (T1 to T5, THE PAIR STAMP of 2026-10-06 (host\fg_stills.h, --fgstamp=1; the fake client's mode 21, fgpipe_stills.h), stood here:
        # out with it in round 8's deep strip, 2026-10-26, as the tool they proved; docs\INSTRUMENTS-FG.md)
    else:
        print("SESSIONS H, L2 to L6: NOT RUN on the third build's helper (the levers are the development build's)")

    # ---- W1, W2, W3: the queued wait on fgPrep and its release, on real fences (stage 10; no lever, so both helpers) -------
    for sid, mode, title in (("W1", 7, "THE QUEUED WAIT, the helper held in a prepare; the client's release"),
                             ("W2", 8, "THE QUEUED WAIT, the helper SUSPENDED; the watch's term and its order"),
                             ("W3", 9, "THE QUEUED WAIT, the helper KILLED; the client's release")):
        Wn = session(sid, "%s (--fake-fg=%d)" % (title, mode), "full", ["--fg=1"], ["--fake-want-fg=3", "--fake-fg=%d" % mode]); n_sessions += 1
        clean(Wn, FULL_RUN + WAIT_RUN[mode])
        if mode != 9:
            ctl = [l for l in Wn["p"] if "THE QUEUED WAIT" in l and "THE CONTROL holds" in l and "did not run, as it must" in l]
            proof(len(ctl) == 1, sid, "the control's line is there and passed (with no release the body behind the wait does not run)")
        else:
            top = [l for l in Wn["p"] if "THE HELPER KILLED" in l and "AT THE TOP (fgPrep reads 0xFFFFFFFFFFFFFFFF, the frame's in-fence 0xFFFFFFFFFFFFFFFF)" in l and "the body ran with no release at all" in l]
            proof(len(top) == 1, sid, "the measurement's line is there and passed (a dead helper's fences stand at the top and hold nothing)")
        want_rel = {7: "THE CLIENT'S RELEASE, the helper alive: fgPrep signalled from the CPU to one past the top (it reads 35), and the body behind the wait ran",
                    8: "THE WATCH'S ORDER, the helper ended first: the body behind the wait ran",
                    9: "THE HELPER GONE: the body behind the wait ran"}[mode]
        rel = [l for l in Wn["p"] if want_rel in l]
        proof(len(rel) == 1, sid, "the release's line is there and passed (%s)" % want_rel.split(":")[0])
        if mode == 8:
            cond = [l for l in Wn["p"] if "THE WATCH'S TERM condemns the suspended helper" in l]
            fact = [l for l in Wn["lines"] if "with the helper still suspended the release's signal" in l]
            proof(len(cond) == 1 and len(fact) == 1, sid, "the watch's term condemned the suspended helper, and the release tried while it was frozen is on record: %s" % (fact[0].strip() if fact else "NO LINE"))
        if mode == 7:
            closed_by(Wn, "the game", 34)
            line = re.search(r"frame generation: this connection: ([^\n]*)", Wn["log"])
            want = "1 set(s) built, 0 refused; 'Q' 2, 'G' 1, dispatched 1, answered with no frame 0; DUPLICATE 0, DRIFT 0; resets by reason: cut 0, build 1, device 0, gap 0, the helper's own 0; pipelined 0"
            proof(line is not None and line.group(1).strip() == want, sid, "the helper's own count is what the client's lines wrote: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
        else:
            proof(not Wn["closing"], sid, "the helper wrote no closing line (this run ended it where it stood)")

    # ---- W4: wire 2 (the wire's one revision, 2026-10-06; no lever, so both helpers) ----------------------------------------
    W4 = session("W4", "WIRE 2: a set with no HUD-less image, a reason on 'G' alone, PIPELINED (--fake-fg=10)", "full", ["--fg=1"],
                 ["--fake-want-fg=3", "--fake-fg=10"] + ([] if REL else ["--fake-fg-stamp"])); n_sessions += 1
    clean(W4, FULL_RUN + W2_RUN + (0 if REL else 1))
    closed_by(W4, "the game", W2_FRAMES)
    line = re.search(r"frame generation: this connection: ([^\n]*)", W4["log"])
    proof(line is not None and line.group(1).strip() == W2_COUNTS, "W4", "THE HELPER'S OWN COUNT is what the fake client sent, a cut said on 'G' alone and one said on both counted ONCE each, PIPELINED once: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
    two = [float(v) for v in re.findall(r"frame generation: created at 3840x2160 \(guides 3840x2160, format 10, scRGB\)[^\n]*, two images [\d.]+ MiB;[^\n]*this process's video memory [\d.]+ -> ([\d.]+) MiB", W4["log"])]
    three = re.findall(r"frame generation: created at 3840x2160 \(guides 3840x2160, format 10, scRGB\)[^\n]*, three images ([\d.]+) MiB;", W4["log"])
    twob = re.findall(r", two images ([\d.]+) MiB;", W4["log"])
    cyc2 = two[1:21]
    proof(len(two) == 21 and len(three) == 1 and len(cyc2) == 20 and max(cyc2) - min(cyc2) <= 64.0, "W4",
          "the helper made 21 sets of TWO images and one of three, by its own lines, and its video memory after each of the twenty creates is flat: %s" % (
              ("%.1f to %.1f MiB over %d creates (%d two-image sets, %d three-image)" % (min(cyc2), max(cyc2), len(cyc2), len(two), len(three))) if cyc2 else "NO CREATE LINES (%d two-image, %d three-image)" % (len(two), len(three))))
    proof(bool(twob) and bool(three) and abs(float(three[0]) * 2.0 - float(twob[0]) * 3.0) < 0.5, "W4",
          "and the video memory rule counted two images for such a set: %s MiB against %s for three" % (twob[0] if twob else "?", three[0] if three else "?"))
    says(W4, r"(?i)DUPLICATE: |DRIFT: |RAISED|did not retire its allocator", "a pair out of step, a raise or a ring that refused a list", want=False)

    # ---- X1 to X5: wire 3 (2026-10-18; the head says what each is) -------------------------------------------------------
    n_sessions += wire3_sessions(cap)
    if REL: n_sessions += own_sessions(cap, B)   # (2026-10-25: O1 to O5, the mod's own library by default and its gate)
    n_sessions += twin_sessions(cap)             # (round 5, H: X6, the second input set and its guides)
    n_sessions += wine_sessions(cap)             # (round 7, L: WF1 to WF4, under Wine the game's own fences)

    # ---- E1: lever 2 (2026-10-10): 'Q' right behind 'F', the helper held ahead of that prepare (a development lever) ----------
    if not REL:
        E1 = session("E1", "LEVER 2: 'Q' RIGHT BEHIND 'F' with the helper held ahead of that prepare: the frame's own answer does not wait (--fake-fg=11)", "full",
                     ["--fg=1", "--fghold_q=2"], ["--fake-want-fg=3", "--fake-fg=11"]); n_sessions += 1
        clean(E1, FULL_RUN + EQ_RUN)
        closed_by(E1, "the game", EQ_FRAMES)
        says(E1, r"--fghold_q: this helper sleeps 8 s before the prepare of pair 34 \(its 'Q' number 2 of the connection\)", "that its lever held the prepare of pair 34, by the helper's own log")
        ans = [l for l in E1["p"] if "THE FRAME'S OWN ANSWER DID NOT WAIT FOR IT" in l and "fgPrep read 33 and stood there" in l]
        proof(len(ans) == 1, "E1", "the order's line is there and passed (the out-fence at 34 and the home retired while fgPrep stood at 33)")
        line = re.search(r"frame generation: this connection: ([^\n]*)", E1["log"])
        proof(line is not None and line.group(1).strip() == EQ_COUNTS, "E1", "THE HELPER'S OWN COUNT is what the fake client sent, in both orders: %s" % (line.group(1).strip() if line else "NO LINE FOR THE CONNECTION"))
        says(E1, r"(?i)DUPLICATE: |DRIFT: |RAISED|did not retire its allocator", "a pair out of step, a raise or a ring that refused a list", want=False)
    else:
        print("SESSION E1: NOT RUN on the third build's helper (its hold is the development build's lever)")

    # ---- R1 to N2c: the helper's two fixes owed before a generated frame is shown (2026-10-10) ---------------------------
    n_sessions += fix_sessions()

    # ---- I: an Agility runtime beside the helper --------------------------------------------------------------------------
    i_base = len(passes)
    if have_agility:
        I = session("I", "DLSS, asked, an Agility SDK runtime beside the helper", "agility", ["--fg=1"], ["--fake-want-fg=1", "--fake-hello-only"]); n_sessions += 1
        clean(I, HELLO_RUN)
        says(I, r"D3D12: Agility SDK runtime 1\.\d+\.\d+ asked", "that the Agility runtime was asked (the instrument's own condition)")
        says(I, r"frame generation: not offered with an Agility SDK runtime beside the helper", "why frame generation is not offered there")
    else:
        print("SESSION I: NOT RUN (no Agility runtime at %s)" % AGILITY)
    i_proofs = len(passes) - i_base

    # ---- K: the caps control ------------------------------------------------------------------------------------------------
    K = session("K", "THE CAPS CONTROL: not asked, read as if it must be offered: the fake client must FAIL that one proof", "full", ["--backend=1"], ["--fake-want-fg=3", "--fake-hello-only"]); n_sessions += 1
    k_ok = K["rc"] != 0 and K["last"] == "FAIL" and len(K["f"]) == 1 and "the handshake says of frame generation what this session must" in K["f"][0]
    print("  %s  [K] the caps control: %s" % ("PASS" if k_ok else "FAIL", "the one proof that had to fail failed, and nothing else did" if k_ok else "IT DID NOT FAIL AS IT MUST (%d FAIL lines, exit code %d)" % (len(K["f"]), K["rc"])))

    counted = len(passes) - i_proofs
    ok = not fails and k_ok and counted >= MIN_PROOFS and len(control) == 1
    for sid, l in fails[:12]: print("  failed: [%s] %s" % (sid, l.strip()[:200]))
    print("%d proofs passed (the floor is %d), %d failed; the control %s; the caps control %s; %d sessions%s" % (
        counted, MIN_PROOFS, len(fails), "held" if len(control) == 1 else "IS NOT THERE", "held" if k_ok else "DID NOT HOLD", n_sessions,
        ("; and %d more proofs in session I (an Agility runtime)" % i_proofs) if have_agility else "; session I NOT RUN (no Agility runtime on this machine)"))
    print("PASS" if ok else "FAIL (the folder is kept: %s)" % OUT)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
