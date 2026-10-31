#!/usr/bin/env python3
"""Frame generation's product, COMPILED WITH THE SWITCH THAT ARMS IT, into %TEMP%. Nothing of the build is touched.

    python tools/test-fg-arms/run.py            the two armed shapes and the two controls
    python tools/test-fg-arms/run.py --keep     keep the objects, the binaries and the maps it made

WHY. Since phase 3's stage 8 the product has a binding that arms, FgPA (source/fg/fg_arm.h), and everything it needs: the
armed seams over fg_vklines.h, the five threads on fg_threads.h's loops, HOLD and SAVE, the conditions, the plan, the call
sites behind FGA(...). SINCE ROUND 9'S STRIP (2026-10-26) IT IS THE ONLY ONE: the development build is the armed one, and the
shape 1.1 ships is the armed release; round 9's sweep retired the switch DLSSIV_FG_ARMS that chose it, and the source refuses a
build that still names it. This tool compiles
and links DLSS-IV.cpp, into %TEMP%, in the two configurations that carry frame generation's product code (the development
one as build.ps1 builds it, and the release with frame generation, build.ps1 -Ship's), and holds what each must carry.

WHAT IT PROVES, per armed shape:
  1. it compiles and links with no error (every seam of the merged binding's contract is there, with its type);
  2. THE MERGED PATH IS IN THE BINARY, OVER FgPA: the linker's map names the machine's armed functions instantiated over
     that binding (a floor of them: the compiler inlines some), and the product's FGA entry points;
  3. the shadow shape is NOT in it (no FgMachine<FgP> function): the product is one machine, not two.
THE CONTROLS: the plain release (no frame generation) must show NONE of point 2's names, lines or the HUD-less picture's; and
the retired switch, named at 0, must NOT COMPILE, refused by the source's own #error (the unarmed product is gone).

THE FORCED DRIVER (phase 3, stage 10) AND THE ARMED DEVELOPMENT LINES (the window lines, the HUD-less picture's check and its
form of before) WENT IN ROUND 8'S DEEP STRIP (docs/INSTRUMENTS-FG.md): NO SHAPE may hold a line or a function of them now,
the armed development build among them (until then it alone held them). And build.ps1 must pass no retired switch.

It builds nothing into the project folder and is not a step of the gate until the main session adds it.
"""
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "source", "DLSS-IV.cpp")
VKINC = os.path.join(ROOT, "external", "vulkan")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-arms")

# The machine's functions only a binding that arms on the wire compiles (fg_machine.h, "THE MERGED PATH").
ARMED = ["ArmedFirePresent", "ArmedPresent", "ArmedPlacerStep", "ArmedFire", "ArmedFireNote", "ArmedBeforeInject", "ArmedSpan",
         "ArmedG", "ArmedDrop", "ArmedFireN", "ArmedFireI", "ArmedKTake", "ArmedResolveI", "NoteFire", "Pace", "MarkEnd", "BeforeBuild"]
ARMED_FLOOR = 8          # of the names above, in the map (the rest were inlined into their one caller)
# The product's own functions that exist only where something calls them: the FGA entry points and the armed threads' mains.
ENTRIES = ["FgpBeforeBuild", "FgpOnResetDone", "FgpInlineGuard", "FgpArmWindowStep", "FgpWaiterSample", "FgpNoSetNow",
           "FgpArmPlacerMain", "FgpArmStamperMain", "FgpArmAtBuild", "FgpArmInstall"]
ENTRY_FLOOR = 2          # (2 since round 5, 2026-10-24: the armed development shape inlines FgpArmAtBuild too; the machine's 96 functions and the armed lines are the proof) (the small ones are inlined into their one caller: the two thread mains, taken by address, and FgpArmAtBuild stay. 4 until 2026-10-14: FgpArmInstall, which has one caller, stayed out of line too, and the armed development shape inlines it since the order's own fence is made in FgpArmVkMake; the armed third build still names all four)
# A line only the armed product can say, by its bytes in the binary: the conditions' table is reached from the FGA sites alone.
ARMED_TEXT = [b"frame generation: standing by while a menu is up", b"frame generation: off: the game runs in exclusive fullscreen",   # (fg18-fix, F10: "parked" is no player's word)
              b"a placer and four stampers, asleep until it is engaged",
              b"frame generation law: the windows start"]   # (phase 5, P1: the law, the armed product's own driver, in both armed shapes)
# THE FORCED DRIVER'S OWN (stage 10), and the armed development lines: lines and functions no shape has since round 8's deep strip.
FORCED_TEXT = [b"frame generation forced: ENDED ITSELF behind window", b"frame generation forced: ENGAGED behind window",
               b"frame generation forced: the windows start", b"frame generation armed: window %ld %s: pairs armed",
               b"frame generation armed: the session's count %s: pairs armed",
               b"frame generation law: window %ld: %s, band %s",
               b"L (the law: ON)", b"L (the law: STANDBY)"]   # (round 3, item 9: the armed window line's state word under the law)   # (phase 5, P1: the law's window line, the armed development build alone)
FORCED_FN = ["FgpForceWindowStep", "FgpArmTotalsText", "FgpArmWindowLine", "FgpForceParkGuard", "FgpArmCountsRead"]
# THE FGA CALL SITES, by file: the callee and how many sites name it. A site that is deleted, or a new one nobody listed, fails.
SITES = {"source/DLSS-IV.cpp": {"FgpOnResetDone": 1, "FgpWaiterResync": 1, "FgpOwnsEnd": 1, "FgpWaiterSample": 1, "FgpArmWindowStep": 1, "FgpWantsRefresh": 1,
                                "fgEnd": 2, "fgTook": 2,
                                "FgpLawOwnsLadder": 1,    # (phase 5, P1: the adaptive step's gate, the law holding the ladder)
                                # (2026-10-20, SDR: the frame's end, FusionFix's point copy, its draw in hkDrawPrim and hkDrawPrimUP; fg_hud.h, "SDR")
                                "FgHudSdrFrame": 1, "FgHudSdrCgaArm": 1, "FgHudSdrCgaDraw": 2, "fgSdrWatch": 1, "fgSdrCgaArmed": 2},
         # (2026-10-12, the save's question: the transport's one raw write into the back buffer tells the device thread's
         # tracker, devthread/dt_bbs.h)
         # (2026-10-12, the phone: the armed product's one question at the interface's boundary, its declaration above
         # IvtOnDraw and its call in it; source/fg/fg_hud.h, FgHudOverlay3d)
         # (2026-10-14, the helper's answer on a card that is the limit: the injection body says, behind the same-frame
         # home's own submit, that this frame's home stands behind its wait for the helper; source/fg/fg_arm.h, FgPA::HomeWaited)
         # (2026-10-20, SDR: its one call behind HDR's block in IvtOnDraw; the flags and the declarations stand at DLSS-IV.cpp's top since round 2)
         "source/transport/DlssIvTransport.h": {"FgpBeforeBuild": 1, "FgpInlineGuard": 2, "DtBbsRawWrite": 1, "FgHudOverlay3d": 2, "FgpHomeWaited": 1,
                                                "fgSdrHooks": 1, "FgHudSdrOnDraw": 1},
         "source/fg/fg_client.h": {"FgpNoSetNow": 1, "FgpSetRefused": 1, "FgHudSetWanted": 1},
         # (stage 15, the HUD-less picture: the encode hands the pass what it drew with, the copy at the interface's
         # boundary is armed for an engaged frame, and what the pass kept goes before a Reset)
         "source/hdr_encode.h": {"FgHudCapture": 1, "FgHudWantsCopy": 2, "FgHudRelease": 1, "FgHudSecond": 2}}
# THE HUD-LESS PICTURE (stage 15; source/fg/fg_hud.h and fg_machine.h's "THE HUD-LESS PICTURE"): in BOTH armed shapes the
# second target's functions and its one line; the check of its copy point, its window lines and the pass of before in NO
# shape since round 8's deep strip; in the control none of it.
HUD_MACHINE = ["ArmedHudBack", "ArmedHudSpan", "ArmedHudStore", "ArmedHudDropped"]
HUD_FN = ["FgHudCapture", "FgHudWant", "FgHudRelease", "FgHudSecond", "FgHudTargetMake", "FgHudTargetGive", "FgHudTargetTick", "FgHudMrtShader", "FgHudOverlay3d",
          "FgHudSdrOnDraw", "FgHudSdrCgaDraw", "FgHudSdrFrame"]   # (2026-10-20, SDR)
HUD_FN_FLOOR = 2         # (the small ones are inlined into their one caller: the pass itself and the shader's maker stay)
HUD_TEXT = [b"frame generation: the encode with the HUD-less picture as its second output (%s%s) %s"]
HUD_DEV_TEXT = [b"frame generation armed: the HUD-less picture, window %ld", b"the check of its HUD-less picture's copy point could not be created",
                b"frame generation: the HUD-less pass (%s%s) %s", b"frame generation armed: the HUD-less picture's second target, window %ld",
                b"frame generation armed: the HUD-less picture in SDR, window %ld", b"the check of its HUD-less picture's copy point in SDR could not be created"]   # (2026-10-20, SDR)
HUD_DEV_FN = ["FgHudCheckTick", "FgHudWindowLine", "FgHudPass", "FgHudDraw", "FgHudUndo", "FgHudShader", "FgHudSecondLine", "FgHudSdrCheckTick", "FgHudSdrLine"]
# THE SHIPPED DESIGN (round 6, W, 2026-10-24; source/fg/fg_ship.h): in BOTH armed shapes the shipped key fg_min_fps is read (its name, the
# parse's own literal, whole between two zero bytes: a condition's line names "fg_min_fps=30" in every armed shape, which must not count) and the fixed mode's one warnings line is there; in the control neither. THE FILL is in NO shape since round 8's deep strip (the law's
# fill step went in its batch 9, the machine's grid fill, held-over real frame and second input set in its batch 10): none of its lines
# or functions in any shape, the armed development build's included.
SHIP_TEXT = [b"\x00fg_min_fps\x00", b"frame generation: the display runs at a fixed %d Hz, so the game is held to %d fps, half the refresh rate, and shown at %d fps with a generated frame between every two real frames",
             b"frame generation: off on this display: it runs at a fixed %d Hz, so the game would be held to %s fps, half the refresh rate, under the %d fps frame generation needs%s"]
FILL_TEXT = [b"frame generation law: the true fill: the machine can make %d", b"frame generation law: the fill: %s (%d step(s) at most)"]
FILL_FN = ["FgpFillWindowStep", "FgFillStep", "FgFillGo", "FgFillStayUs", "FgFillShownExpPm",
           "ArmedFillSlot", "ArmedFillI2", "ArmedHeldFirst", "ArmedHeldFireOne", "ArmedInQ", "ArmedInMay", "PacePattern", "RecCopyGuides"]


def vcvars():
    root = os.environ.get("DLSSIV_VSROOT")
    if not root:
        vw = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), "Microsoft Visual Studio", "Installer", "vswhere.exe")
        if os.path.exists(vw):
            r = subprocess.run([vw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                                "-property", "installationPath"], capture_output=True, text=True)
            if r.returncode == 0 and r.stdout.strip():
                root = r.stdout.strip().splitlines()[0].strip()
    if not root:
        root = r"C:\Program Files\Microsoft Visual Studio\2022\Community"
    p = os.path.join(root, "VC", "Auxiliary", "Build", "vcvars32.bat")
    if not os.path.exists(p):
        raise SystemExit("vcvars32.bat not found at " + p)
    return p


def build(name, defines):
    """Compile and link into OUT\\<name>.*, with build.ps1's own flags and no resource. Returns (exit code, output, map text)."""
    obj, asi, mp = (os.path.join(OUT, name + e) for e in (".obj", ".asi", ".map"))
    for f in (obj, asi, mp):
        if os.path.exists(f):
            os.remove(f)
    d = " ".join("/D" + x for x in defines)
    cmd = ('"%s" >nul 2>&1 && cl /nologo /LD /MT /GS /EHa /W3 /std:c++17 /O2 %s /DWIN32 /D_WINDOWS /DIVT_BUILD_ID=\\"\\"\\"test-fg-arms\\"\\"\\" '
           '/I"%s" "%s" /Fe:"%s" /Fo:"%s" /link /SUBSYSTEM:WINDOWS /MACHINE:X86 /Brepro /MAP:"%s" d3d9.lib user32.lib version.lib'
           % (vcvars(), d, VKINC, SRC, asi, obj, mp))
    bat = os.path.join(OUT, name + ".cmd")
    with open(bat, "w", newline="\r\n") as f:
        f.write("@echo off\n" + cmd + "\n")
    r = subprocess.run(["cmd.exe", "/c", bat], capture_output=True, text=True, errors="replace")
    text = ""
    if os.path.exists(mp):
        text = open(mp, "r", errors="replace").read()
    return r.returncode, (r.stdout or "") + (r.stderr or ""), text


def fga_sites():
    """Every FGA(...) in the three files that hold call sites, by the names inside it. Returns the list of what is wrong."""
    bad, total = [], 0
    for rel, want in SITES.items():
        text = open(os.path.join(ROOT, *rel.split("/")), "r", encoding="utf-8", errors="replace").read()
        text = re.sub(r"//[^\n]*", "", text)
        body = []
        for m in re.finditer(r"\bFGA\(", text):
            if text[max(0, m.start() - 8):m.start()].endswith("#define "):
                continue
            depth, j = 1, m.end()
            while j < len(text) and depth:
                depth += text[j] == "("
                depth -= text[j] == ")"
                j += 1
            body.append(text[m.end():j - 1])
        total += len(body)
        for name, n in want.items():
            got = sum(1 for b in body if re.search(r"\b%s\b" % name, b))
            if got != n:
                bad.append("%s: %d FGA site(s) name %s, %d wanted" % (rel, got, name, n))
        for b in body:
            if not any(re.search(r"\b%s\b" % name, b) for name in want):
                bad.append("%s: an FGA site this tool does not list: FGA(%s)" % (rel, b.strip()[:80]))
    return bad, total


def names(maptext, binding):
    """The machine's functions the map names over one binding: FgMachine<binding>::X is ?X@?$FgMachine@U<binding>@@@@..."""
    return set(re.findall(r"\?(\w+)@\?\$FgMachine@U%s@@@@" % re.escape(binding), maptext))



# (THE ARMED DEVELOPMENT LINES UNDER EITHER DRIVER, phase 5's round 3 item 9, the check that the forced driver's window step and the
# law's called the same window lines: retired in round 8's deep strip with the forced driver and the lines.)


def main():
    keep = "--keep" in sys.argv[1:]
    os.makedirs(OUT, exist_ok=True)
    fails = []
    bad, total = fga_sites()
    print("  the armed path's call sites: %d FGA(...) in %d files, each one listed here by its callee%s" % (total, len(SITES), "" if not bad else ": NOT AS LISTED"))
    fails += bad
    # (ROUND 9'S STRIP, 2026-10-26: build.ps1's -Arms is retired, so the development build, built with no define, is the armed
    # one; ROUND 9'S SWEEP retired the switch itself. Held here: no -Arms left, build.ps1 passes no DLSSIV_FG_ARMS, and the
    # source refuses a build that names it.)
    ps1 = open(os.path.join(ROOT, "build.ps1"), "r", encoding="utf-8", errors="replace").read()
    cpp = open(SRC, "r", encoding="utf-8", errors="replace").read()
    no_arms = "$Arms" not in ps1 and "/DDLSSIV_FG_ARMS" not in ps1 and "#ifdef DLSSIV_FG_ARMS\n#error" in cpp.replace("\r\n", "\n")
    print("  the development build is the armed one: %s" % ("build.ps1 has no -Arms and passes no DLSSIV_FG_ARMS, and DLSS-IV.cpp refuses the switch: every shape below is compiled armed"
                                                        if no_arms else "NOT AS THIS TOOL SPELLS IT"))
    if not no_arms:
        fails.append("build.ps1 still has -Arms or passes DLSSIV_FG_ARMS, or DLSS-IV.cpp does not refuse the retired switch")
    # THE HUD-LESS PASS'S SHADERS (stage 15) live outside source\shaders, where build.ps1's stale-header gate does not look:
    # their headers are held to their sources here, before anything is compiled over them.
    fgsh = subprocess.run(["powershell.exe", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(ROOT, "tools", "compile-fg-shaders.ps1"), "-Verify"],
                          capture_output=True, text=True, errors="replace")
    last = [l for l in (fgsh.stdout or "").splitlines() if l.strip()]
    print("  frame generation's own shaders (tools\\compile-fg-shaders.ps1 -Verify): %s" % (last[-1].strip() if last else "NO OUTPUT"))
    if fgsh.returncode != 0:
        fails.append("the headers under source\\fg\\shaders are not what their sources compile to (tools\\compile-fg-shaders.ps1 regenerates them)")
    # (THE RETIRED SWITCH AT 0, refused: the unarmed product is gone since round 9's strip, and DLSS-IV.cpp says so with an #error)
    rc0, out0, _ = build("control-arms0", ["DLSSIV_DEV=0", "DLSSIV_FG=1", "DLSSIV_FG_ARMS=0"])
    refused = rc0 != 0 and "DLSSIV_FG_ARMS is gone" in out0
    print("  control-arms0  the retired switch at 0: %s" % ("DOES NOT COMPILE, as it must (the source's #error)" if refused else "NOT REFUSED BY THE SOURCE'S #error (exit %d)" % rc0))
    if not refused:
        fails.append("THE CONTROL: DLSSIV_FG_ARMS=0 was not refused by DLSS-IV.cpp's #error")
    shapes = [("armed-dev", [], True),
              ("armed-ship", ["DLSSIV_DEV=0", "DLSSIV_FG=1"], True),
              ("control-release", ["DLSSIV_DEV=0"], False)]
    for name, defs, armed in shapes:
        rc, out, mp = build(name, defs)
        errs = [l for l in out.splitlines() if re.search(r"\berror\b", l)]
        warns = len(re.findall(r"warning C\d+", out))
        if rc != 0 or not mp:
            print("  %-14s DID NOT BUILD (exit %d), %d error line(s):" % (name, rc, len(errs)))
            for l in errs[:25]:
                print("      " + l.strip()[:260])
            fails.append("%s does not compile and link" % name)
            continue
        over_a, over_p = names(mp, "FgPA"), names(mp, "FgP")
        got = sorted(n for n in ARMED if n in over_a)
        ent = sorted(n for n in ENTRIES if re.search(r"\?%s@@" % n, mp))
        asi = open(os.path.join(OUT, name + ".asi"), "rb").read()
        said = [t for t in ARMED_TEXT if t in asi]
        print("  %-14s built (%d warning(s)): the machine over FgPA %d function(s), over FgP %d; of the merged path's %d names %d are in the map (%s);"
              " of the product's armed entry points %d of %d (%s)"
              % (name, warns, len(over_a), len(over_p), len(ARMED), len(got), ", ".join(got) if got else "none", len(ent), len(ENTRIES), ", ".join(ent) if ent else "none"))
        print("  %-14s %d of the %d lines only the armed product says are in the binary" % ("", len(said), len(ARMED_TEXT)))
        forced = [t for t in FORCED_TEXT if t in asi]
        ffn = sorted(n for n in FORCED_FN if re.search(r"\?%s@@" % n, mp))
        print("  %-14s the forced driver: %d of its %d lines in the binary, %d of its %d functions in the map (%s)"
              % ("", len(forced), len(FORCED_TEXT), len(ffn), len(FORCED_FN), ", ".join(ffn) if ffn else "none"))
        if forced or ffn:
            fails.append("%s: %d line(s) and %d function(s) of the forced driver are in a shape, after round 8's deep strip took it out" % (name, len(forced), len(ffn)))
        # THE HUD-LESS PICTURE (stage 15)
        hm = sorted(n for n in HUD_MACHINE if n in over_a)
        hf = sorted(n for n in HUD_FN if re.search(r"\?%s@@" % n, mp))
        ht = [t for t in HUD_TEXT if t in asi]
        hdt = [t for t in HUD_DEV_TEXT if t in asi]
        hdf = sorted(n for n in HUD_DEV_FN if re.search(r"\?%s@@" % n, mp))
        print("  %-14s the HUD-less picture: of the machine's %d functions %d are in the map over FgPA (%s); of the game side's %d functions %d (%s); %d of its %d line(s);"
              " the development check: %d of %d line(s), %d of %d function(s)"
              % ("", len(HUD_MACHINE), len(hm), ", ".join(hm) if hm else "none", len(HUD_FN), len(hf), ", ".join(hf) if hf else "none", len(ht), len(HUD_TEXT),
                 len(hdt), len(HUD_DEV_TEXT), len(hdf), len(HUD_DEV_FN)))
        if armed:
            if not hm or len(hf) < HUD_FN_FLOOR or len(ht) != len(HUD_TEXT):
                fails.append("%s: the HUD-less picture is not whole in an armed shape: %d of the machine's functions, %d of the game side's (floor %d), %d of %d line(s)"
                             % (name, len(hm), len(hf), HUD_FN_FLOOR, len(ht), len(HUD_TEXT)))
            if hdt or hdf:
                fails.append("%s: %d line(s) and %d function(s) of the development check or the form of before are in a shape, after round 8's deep strip took them out" % (name, len(hdt), len(hdf)))
        elif hm or hf or ht or hdt or hdf:
            fails.append("THE CONTROL: the release without frame generation holds something of the HUD-less picture: %d machine function(s), %d of the game side's, %d line(s), %d of the check's"
                         % (len(hm), len(hf), len(ht), len(hdt) + len(hdf)))
        # THE SHIPPED DESIGN AND THE FILL (round 6)
        st = [t for t in SHIP_TEXT if t in asi]
        ft = [t for t in FILL_TEXT if t in asi]
        ff = sorted(n for n in FILL_FN if re.search(r"\?%s@@" % n, mp))
        print("  %-14s the shipped design: %d of its %d literal(s) in the binary; the fill: %d of its %d line(s), %d of its %d function(s) in the map (%s)"
              % ("", len(st), len(SHIP_TEXT), len(ft), len(FILL_TEXT), len(ff), len(FILL_FN), ", ".join(ff) if ff else "none"))
        if armed and len(st) != len(SHIP_TEXT):
            fails.append("%s: %d of %d literal(s) of the shipped design (fg_min_fps read, the fixed mode's warnings line) are in an armed shape" % (name, len(st), len(SHIP_TEXT)))
        if not armed and st:
            fails.append("THE CONTROL: the release without frame generation holds %d literal(s) of the shipped design" % len(st))
        if ft or ff:
            fails.append("%s: %d line(s) and %d function(s) of the fill are in a shape, after round 8's deep strip took it out (%s)" % (name, len(ft), len(ff), ", ".join(ff)))
        if armed:
            if len(got) < ARMED_FLOOR:
                fails.append("%s: only %d of the merged path's functions are in the map over FgPA (floor %d): the armed shape was not compiled whole" % (name, len(got), ARMED_FLOOR))
            if len(ent) < ENTRY_FLOOR:
                fails.append("%s: only %d of the product's armed entry points are in the map (floor %d)" % (name, len(ent), ENTRY_FLOOR))
            if len(said) != len(ARMED_TEXT):
                fails.append("%s: %d of %d lines of the armed product are in the binary: a call site that reaches them compiled to nothing" % (name, len(said), len(ARMED_TEXT)))
            if over_p:
                fails.append("%s: %d function(s) of the machine are still compiled over FgP, the binding that cannot arm (%s ...)" % (name, len(over_p), ", ".join(sorted(over_p)[:4])))
        else:
            if said:
                fails.append("THE CONTROL: the release without frame generation holds %d line(s) only the armed product says" % len(said))
            if over_a or over_p or got or ent:
                fails.append("THE CONTROL: the release without frame generation names %d function(s) of the machine and %d armed entry point(s) (%s)"
                             % (len(over_a) + len(over_p), len(ent), ", ".join(sorted(over_a)[:4] + ent[:4])))
            else:
                print("  the control: the release without frame generation names NO function of the machine and no armed entry point, where the same reader found them in the armed shapes")
    if not keep:
        for f in os.listdir(OUT):
            try:
                os.remove(os.path.join(OUT, f))
            except OSError:
                pass
    for f in fails:
        print("  FAIL: " + f)
    print("FAILED" if fails else "PASS")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
