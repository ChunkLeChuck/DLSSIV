r"""THE TWO ALTERED BUILDS: TEST LIBRARIES THAT MUST MAKE ANOTHER PICTURE (2026-10-06, stage 1a's review, the
skeptic's "the pass that proves nothing" and its list b5; docs\reports\fg-3\own-build-2026-10-06\stage-1a-review).

The next stage compares the picture our library generates with the one AMD's library generates. Equal pictures prove
something only if a library that is NOT the same would have made them unequal ON THE VERY PAIRS COMPARED. Two things in
AMD's code make an equal picture easy to get for nothing: optical flow's search answers "scene changed" for its first
frames, and the interpolation ignores optical flow for 10 frames after a reset; and on a still or constant picture 7 of
the 18 pipelines never reach the picture at all. So the compare needs two controls it must FAIL, one for each half of
the effect, and this script builds them:

    ffx-own-fg-alt-fi.dll   ONE constant changed in the INTERPOLATION shader
    ffx-own-fg-alt-of.dll   ONE constant changed in the OPTICAL FLOW shader, in the search

    python tools\ffx-own\build_alt.py              both, into obj\ffx-own-alt (ignored by git; never committed, never packaged)
    python tools\ffx-own\build_alt.py fi           one of them (fi or of)
    python tools\ffx-own\build_alt.py --out <dir>  into another folder

SINCE THE OWN BUILD'S STAGE 2 (2026-10-07) EACH IS MADE TWICE FROM THE ONE ALTERED TREE: as above, and as the second
library, the mod's own interface over the same altered source (build-ffx-own.ps1 -Target dlssiv-fg -TestName ...):

    dlssiv-fg-alt-fi.dll, dlssiv-fg-alt-of.dll     into obj\dlssiv-fg-alt (--out2 <dir> for another folder; --no-second leaves them out)

The second library's picture compare must fail on them as the first's fails on the first two.
SINCE STAGE 3 (2026-10-10) THOSE TWO CARRY THE PRODUCT'S CUTS (ffx-own\cuts\CUTS.txt, the ones marked "in"), applied on
top of the altered line by the build script, as the product carries them: the controls are then controls of the very
passes the product runs. A cut that edits one of the two altered lines stops the build (its old text is gone), and
that is right: the control would have to be written again. The judge's third control, one pixel changed
(dlssiv-fg-px.dll), is not made here: it is a cut of its own (build-ffx-own.ps1 -Target dlssiv-fg -TestName px).

THE TWO CHANGES, each one line of AMD's, made in a TEMP COPY of the kit and never in external\ffx\ffx_api_2.3.0:

  alt-fi  framegeneration/fsr3/include/gpu/frameinterpolation/ffx_frameinterpolation.h, line 86, in
          computeInterpolatedColor:
              const FfxFloat32x2 fUvInScreenSpace       = (FfxFloat32x2(iPxPos) + 0.5f) / DisplaySize();
          0.5f becomes 1.5f. This is the position at which the pass reads the previous and the current picture (along
          the game's vectors and along optical flow's): with 1.5 every read lands exactly one display pixel to the right
          and one down. So every interpolated pixel takes the colour its lower right neighbour should have had, and the
          generated picture differs wherever the picture is not perfectly flat, moving or not, on every dispatch that
          interpolates (all but the first after a reset). It does not depend on the vectors being wrong, on optical
          flow, or on the two frames differing: a blend factor would have changed nothing where both frames agree.
          Only the interpolation pass includes this file: its two headers change (cs_6_2 and cs_6_6) and no other.

  alt-of  framegeneration/fsr3/include/gpu/opticalflow/ffx_opticalflow_compute_optical_flow_v5.h, line 174, in
          DecodeSearchCoord (the branch that is compiled, FFX_OPTICALFLOW_FIX_TOP_LEFT_BIAS == 1):
              FfxInt32 dx = FfxInt32(bits & 0xfu) - SearchRadiusX;
          0xfu becomes 0x7u. The search tries 16 by 16 offsets round its prediction and packs the best one's x in 4
          bits (0 to 15, where 8 is "no offset"); with the mask 7 every best offset of 0 or more to the right decodes 8
          pixels too far left, at each of the 7 pyramid levels, and the next finer level starts from that wrong vector.
          So wherever something moves, optical flow's vectors are wrong by many pixels. Where nothing moves they end
          as AMD's do (at the finest level AMD's own fallback writes a zero vector when the block has not moved).
          WHAT THIS CONTROL CAN AND CANNOT DO, and it is the reason it exists: optical flow reaches the generated
          picture only where the interpolation takes its colour, which is where the two frames match WORSE along the
          game's vectors than along optical flow's (ffx_frameinterpolation.h 160 to 170: motion the game's vectors do
          not describe), and only from the 10th frame after a reset. On such pixels this library must give another
          colour. On a scene whose motion the game's vectors describe everywhere, on a still picture and on the first
          frames after a reset it gives the SAME picture as the unaltered library, and that is not a fault of the
          control: it says that the compare, on that picture, proves nothing about the 7 optical flow pipelines.
          The test picture then needs something that moves without vectors.
          Only the search pass includes this file: its two headers change and no other.

HOW A TEST LIBRARY IS MADE. A temp tree gets copies of build-ffx-own.ps1, the kit and the glue; the one line is changed
in the copy (the script stops if the line is not there exactly once); gen_shaders.py, with the same pinned dxc and the
same checks, makes the 72 headers from the changed copy, and exactly the two headers named above must differ from the
committed ones; the temp tree is marked with ffx-own\ALTERED.txt; and the COPY of the build script is run with
-TestName, which wants that mark (build-ffx-own.ps1 says why: the unaltered source can never come out under a test's
name, and a marked tree can never make ffx-own-fg.dll). The mark is delivered beside the library as <its name>.txt.
A test library that came out as the record's library, or with another list of exports, stops the script.

Nothing here runs on a GPU. Everything runs at BelowNormal priority.
"""
import ctypes, hashlib, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import ffx_record as R
import gen_shaders as G
import pe_read

ALT = {
    "fi": dict(
        name="ffx-own-fg-alt-fi", test="alt-fi", line=86, where="computeInterpolatedColor",
        file="framegeneration/fsr3/include/gpu/frameinterpolation/ffx_frameinterpolation.h",
        old=b"    const FfxFloat32x2 fUvInScreenSpace       = (FfxFloat32x2(iPxPos) + 0.5f) / DisplaySize();",
        new=b"    const FfxFloat32x2 fUvInScreenSpace       = (FfxFloat32x2(iPxPos) + 1.5f) / DisplaySize();",
        passes=["ffx_frameinterpolation_pass"],
        must="every interpolated pixel reads the two pictures one display pixel to the right and one down: the generated picture differs wherever the picture is not flat, "
             "on every dispatch that interpolates (all but the first after a reset)"),
    "of": dict(
        name="ffx-own-fg-alt-of", test="alt-of", line=174, where="DecodeSearchCoord",
        file="framegeneration/fsr3/include/gpu/opticalflow/ffx_opticalflow_compute_optical_flow_v5.h",
        old=b"    FfxInt32 dx = FfxInt32(bits & 0xfu) - SearchRadiusX;",
        new=b"    FfxInt32 dx = FfxInt32(bits & 0x7u) - SearchRadiusX;",
        passes=["ffx_opticalflow_compute_optical_flow_advanced_pass_v5"],
        must="optical flow's vectors are wrong by 8 pixels and more wherever something moves: the generated picture differs on the pixels where the interpolation takes optical "
             "flow's colour (motion the game's vectors do not describe), from the 10th frame after a reset. Where the game's vectors describe all motion it is the SAME picture, "
             "and the compare then proves nothing about optical flow"),
}
# (since the own build's stage 3 the build script applies the product's cuts to the second library: the list of cuts, each
# cut, and the three scripts that read and compile them go into the temp tree with it. In an altered tree the headers of
# the cuts are compiled from the altered and cut copy, never taken from ffx-own\shaders-cut.
# SINCE 2026-10-12 THOSE ARE THE EXACT LIBRARY'S CUTS, the ones marked "in" (build-ffx-own.ps1: a -TestName build stages
# "exact"): the product's picture is its own since that day, and an altered library with a picture cut in it would
# differ from AMD's whatever was altered.)
BUILD_INPUTS = ["build-ffx-own.ps1", os.path.join("external", "ffx", "ffx_api_2.3.0"), os.path.join("ffx-own", "glue"), os.path.join("ffx-own", "cuts"),
                os.path.join("tools", "ffx-own", "cuts.py"), os.path.join("tools", "ffx-own", "gen_shaders.py"), os.path.join("tools", "ffx-own", "dxbc.py")]
FIVE = ["ffxConfigure", "ffxCreateContext", "ffxDestroyContext", "ffxDispatch", "ffxQuery"]
SEVEN = ["dlssivFgCreate", "dlssivFgDestroy", "dlssivFgInfo", "dlssivFgInterpolate", "dlssivFgOpticalFlow", "dlssivFgPrepare", "dlssivFgStamps"]   # the second library's own names


class Stopped(Exception):
    pass


def build(which, out, env=None, out2=None):
    """makes one test library in the folder out; -> its facts. Raises Stopped with one line. With out2 the same altered
    tree also makes the second library's test build there (dlssiv-fg-alt-<which>.dll): its facts are under "second"."""
    a = ALT[which]
    tmp = tempfile.mkdtemp(prefix="ffxa-")
    try:
        tree = os.path.join(tmp, "DLSS-IV")
        for rel in BUILD_INPUTS:
            s = os.path.join(ROOT, rel); d = os.path.join(tree, rel)
            if os.path.isdir(s): shutil.copytree(s, d)
            else: os.makedirs(os.path.dirname(d), exist_ok=True); shutil.copy2(s, d)
        kit = os.path.join(tree, "external", "ffx", "ffx_api_2.3.0")
        target = os.path.join(kit, a["file"].replace("/", os.sep))
        d = open(target, "rb").read()
        if d.count(a["old"]) != 1: raise Stopped("%s: the line to alter is in AMD's %s %d time(s), not once: %s" % (a["name"], a["file"], d.count(a["old"]), a["old"].decode().strip()))
        line = d[:d.index(a["old"])].count(b"\n") + 1
        if line != a["line"]: raise Stopped("%s: the line to alter is line %d of %s, this script says %d" % (a["name"], line, a["file"], a["line"]))
        open(target, "wb").write(d.replace(a["old"], a["new"]))
        try: files, facts = G.generate(kit=kit)
        except G.Refused as e: raise Stopped("%s: the generator on the altered copy: %s" % (a["name"], e))
        differ = sorted(f for f in files if open(os.path.join(G.HEADERS, f), "rb").read() != files[f].encode("ascii"))
        want = sorted(p + v + "_permutations.h" for p in a["passes"] for v in ("", "_wave64"))
        if differ != want: raise Stopped("%s: the altered line changed the headers %s, and it must change exactly %s" % (a["name"], differ, want))
        G.write(files, os.path.join(tree, "ffx-own", "shaders"))
        mark = ["%s.dll: A TEST LIBRARY, NEVER THE PRODUCT. It must fail the picture compare." % a["name"],
                "Made by tools\\ffx-own\\build_alt.py from a temp copy of AMD's source with ONE line changed:",
                "file   %s" % a["file"], "line   %d, in %s" % (line, a["where"]), "was    %s" % a["old"].decode().strip(), "is     %s" % a["new"].decode().strip(),
                "so     %s" % a["must"], "headers that differ from the committed ones: %s" % ", ".join(differ)]
        open(os.path.join(tree, "ffx-own", "ALTERED.txt"), "w", newline="\r\n").write("\n".join(mark) + "\n")
        os.makedirs(out, exist_ok=True)
        cmd = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(tree, "build-ffx-own.ps1"), "-TestName", a["test"], "-OutDir", os.path.abspath(out)]
        r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", creationflags=0x08000000, env=env)
        dll = os.path.join(out, a["name"] + ".dll")
        if r.returncode != 0 or not os.path.isfile(dll): raise Stopped("%s: build-ffx-own.ps1 -TestName %s: exit code %d: %s" % (a["name"], a["test"], r.returncode, " ".join((r.stdout + r.stderr).split())[-600:]))
        data = open(dll, "rb").read(); sha = hashlib.sha256(data).hexdigest()
        rec = R.read()
        if sha == rec["sha256"]: raise Stopped("%s came out as the record's library (SHA-256 %s): the alteration did not reach it" % (a["name"], sha))
        p = pe_read.PE(dll); own, names = p.exports()
        if own != a["name"] + ".dll" or names != FIVE: raise Stopped("%s calls itself %r and exports %s" % (a["name"], own, names))
        facts = dict(which=which, name=a["name"], path=dll, size=len(data), sha256=sha, file=a["file"], line=line, old=a["old"].decode().strip(), new=a["new"].decode().strip(),
                     headers=differ, note=os.path.join(out, a["name"] + ".txt"))
        if out2:
            # THE SECOND LIBRARY FROM THE SAME ALTERED TREE: the mod's own interface over the altered source
            name2 = "dlssiv-fg-" + a["test"]
            os.makedirs(out2, exist_ok=True)
            cmd = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(tree, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-TestName", a["test"], "-OutDir", os.path.abspath(out2)]
            r = subprocess.run(cmd, capture_output=True, text=True, errors="replace", creationflags=0x08000000, env=env)
            dll2 = os.path.join(out2, name2 + ".dll")
            if r.returncode != 0 or not os.path.isfile(dll2): raise Stopped("%s: build-ffx-own.ps1 -Target dlssiv-fg -TestName %s: exit code %d: %s" % (name2, a["test"], r.returncode, " ".join((r.stdout + r.stderr).split())[-600:]))
            data2 = open(dll2, "rb").read(); sha2 = hashlib.sha256(data2).hexdigest()
            for rp in (R.PATH_FG, R.PATH_CUT):
                rec2 = R.read(rp) if os.path.isfile(rp) else None
                if rec2 and sha2 == rec2["sha256"]: raise Stopped("%s came out as a recorded library (%s, SHA-256 %s): the alteration did not reach it" % (name2, os.path.basename(rp), sha2))
            p2 = pe_read.PE(dll2); own2, names2 = p2.exports()
            if own2 != name2 + ".dll" or names2 != SEVEN: raise Stopped("%s calls itself %r and exports %s" % (name2, own2, names2))
            sib = [l.split() for l in open(os.path.join(out2, "objects.txt"), encoding="ascii").read().splitlines() if l.startswith("sibling ")]
            if len(sib) != 1 or sib[0][1] != sha: raise Stopped("%s: its objects link to a reference library with SHA-256 %s, and the altered reference library built beside it has %s" % (name2, sib[0][1] if sib else "?", sha))
            facts["second"] = dict(name=name2, path=dll2, size=len(data2), sha256=sha2, note=os.path.join(out2, name2 + ".txt"))
        return facts
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    a = sys.argv[1:]
    out = os.path.join(ROOT, "obj", "ffx-own-alt"); out2 = os.path.join(ROOT, "obj", "dlssiv-fg-alt"); which = []
    while a:
        o = a.pop(0)
        if o == "--out" and a: out = os.path.abspath(a.pop(0))
        elif o == "--out2" and a: out2 = os.path.abspath(a.pop(0))
        elif o == "--no-second": out2 = None
        elif o in ALT: which.append(o)
        else: raise SystemExit("usage: build_alt.py [fi] [of] [--out <folder>] [--out2 <folder>] [--no-second]")
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: dxc, cl and link inherit it
    env = {k: v for k, v in os.environ.items() if k.upper() not in ("CL", "_CL_", "LINK", "_LINK_")}
    for w in which or ["fi", "of"]:
        try: f = build(w, out, env, out2)
        except Stopped as e:
            print("FAIL: " + str(e)); return 1
        print("%s  %d bytes  SHA-256 %s" % (f["path"], f["size"], f["sha256"]))
        if "second" in f: print("%s  %d bytes  SHA-256 %s  (the second library from the same altered tree; its objects link to the library above)" % (f["second"]["path"], f["second"]["size"], f["second"]["sha256"]))
        print("  A TEST LIBRARY: %s line %d, \"%s\" is \"%s\"" % (f["file"], f["line"], f["old"], f["new"]))
        print("  headers that differ: %s; what was altered is beside it in %s" % (", ".join(f["headers"]), os.path.basename(f["note"])))
    print("PASS: the test libraries are in %s. They are controls: each must FAIL the picture compare. Never packaged." % out)
    return 0


if __name__ == "__main__":
    sys.exit(main())
