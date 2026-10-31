r"""THE VARIANT LIBRARIES: THE CUTS THAT CHANGE THE PICTURE, EACH BUILT FOR THE OWNER'S EYE AND NEVER INTO THE PRODUCT
(2026-10-11; docs\reports\fg-3\own-build-2026-10-06\stage-3\REPORT.md, "For stage 5").

Stage 3 put into the product every cut that leaves each bit of a generated frame as AMD's. What is left to save changes
the picture, and whether a cheaper picture is still the right picture is the owner's to say, on real frames. So each
such cut (ffx-own\cuts\CUTS.txt: the state "picture") is built into A LIBRARY OF ITS OWN NAME, the product's cuts and
that one on top:

    obj\dlssiv-fg-var\<variant>\dlssiv-fg-v-<variant>.dll      (cuts.txt beside it says what is in it)

and one more with all of them together (the variant "all"). None of them is the product's file, none has a record,
none is packaged: tools\ffx-own\cuts.py refuses a "picture" cut in any build that is not a variant, and a variant
that holds none. They are priced and compared by tools\test-fg-own\variants.py and replayed on the game's captured
frames by tools\fg-replay (its --lib takes a library by its path).

    python tools\ffx-own\build_variants.py              every variant and "all"
    python tools\ffx-own\build_variants.py guide-cap    the named ones
    python tools\ffx-own\build_variants.py --list       the variants, each with its cut

SINCE 2026-10-12, 15:13 THREE OF THESE CUTS ARE THE PRODUCT'S (v-search-half, v-no-inpainting-pyramid, v-env: accepted by the owner in
the game as the candidate "ship"; CUTS.txt: "accepted"). THE VARIANTS STAY WHAT THEY WERE: each is built on THE EXACT LIBRARY'S
cuts (the ones marked "in"), not on the product's, so "env" is still v-env alone and none of them moves when the
product takes a picture cut. "ship" is a frozen library: the bytes the owner ran.

Compiling only: nothing here creates a device. The build script lowers its own priority.
"""
import os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import cuts

# the variant's name (in its file's name) and the "picture" cuts it holds on top of the product's
VARIANTS = [
    ("search-half",           ["v-search-half"]),
    ("filtered-lookups",      ["v-interpolation-filtered-lookups"]),
    ("guide-cap",             ["v-guide-cap"]),
    ("no-inpainting-pyramid", ["v-no-inpainting-pyramid"]),
    ("no-secondary",          ["v-game-field-no-secondary"]),
    ("no-scd",                ["v-no-scene-change-detector"]),
]
VARIANTS.append(("all", [c for _, cs in VARIANTS for c in cs]))
# (2026-10-11) NOT A SAVING BUT A FIX, and so not in "all": the ghost outline behind a thing that stands still on the screen
# over a fast background (ffx-own\cuts\v-ghost.cut; docs\reports\fg-3\sitting-2026-10-11b\BRIEF-ghost.md). "ghost-gate" is
# its instrument: the same library with every pixel of the variant's own path painted one flat colour (the gate's map).
# "alone" and "ghost-alone" are the product and the variant with nothing inpainted (the interpolation pass's own picture):
# the two differ exactly where the variant's own path ran. tools\test-fg-own\ghost.py holds them against the gate's map.
FIXES = [
    ("ghost",                 ["v-ghost"]),
    ("ghost-gate",            ["v-ghost", "v-ghost-show-gate"]),
    ("alone",                 ["v-show-interpolation-alone"]),
    ("ghost-alone",           ["v-ghost", "v-show-interpolation-alone"]),
]
# (2026-10-11) THE SAME GHOST, FOR THE WHOLE PICTURE: each sample relit for the place it is put, by two envelopes of the real
# frames' log brightness kept at the screen place (ffx-own\cuts\v-env.cut; docs\reports\fg-3\desk-2026-10-11-ghost\BRIEF-env.md).
# From the product, NOT on top of v-ghost. "env-gate" paints where it acts and "env-alone" is it with nothing inpainted
# (tools\test-fg-own\env.py holds "env-alone" against "alone" and the map); "env-brief" is the desk's own numbers and
# "env-quarter" the envelopes at a quarter of the display's size, both to compare with.
# (2026-10-12) v-env.cut HAS A NEW TEXT (docs\reports\fg-3\sitting-2026-10-12\BRIEF-lib4.md: what was left of the ghost, and
# its cost); the text the owner ran in the game that morning is v-env-1.cut, and "env-brief" and "env-quarter" stay
# what they were: that text with the desk's numbers, and with the envelopes at a quarter. "env-need" paints the gains;
# "env-less-creep", "env-less-apart" and "env-less-moved" are v-env with one of its three mends taken out each.
FIXES += [
    ("env",                   ["v-env"]),
    ("env-gate",              ["v-env", "v-env-show-gate"]),
    ("env-alone",             ["v-env", "v-show-interpolation-alone"]),
    ("env-need",              ["v-env", "v-env-show-need"]),
    ("env-less-creep",        ["v-env", "v-env-less-creep"]),
    ("env-less-apart",        ["v-env", "v-env-less-apart"]),
    ("env-less-moved",        ["v-env", "v-env-less-moved"]),
    ("env-brief",             ["v-env-1", "v-env-brief"]),
    ("env-quarter",           ["v-env-1", "v-env-quarter"]),
]
# (2026-10-11) TWO INSTRUMENTS FOR THE TORN BOTTOM OF THE TYRE: what the interpolation pass holds of a pixel, as numbers in the
# picture (the game field's vector and which samples are used; optical flow's vector and its share), on the product
# and on v-env. tools\test-fg-own\tyre.py reads them.
FIXES += [
    ("fields",                ["v-show-fields"]),
    ("flow",                  ["v-show-flow"]),
    ("env-flow",              ["v-env", "v-show-flow"]),
]
# (2026-10-12) THE PICTURE CUTS, EACH ALONE ON TOP OF v-env, for the judgement on the owner's captured frames
# (tools\test-fg-own\lib4.py cuts: against v-env, by the three-replay rule); and THE CANDIDATE, "ship": the product, v-env
# and every picture cut that judgement let through. docs\reports\fg-3\sitting-2026-10-12\lib4\README.md says which, and why.
FIXES += [("env-" + name, ["v-env"] + cs) for name, cs in VARIANTS[:6] if name != "no-secondary"]
VARIANTS += FIXES
# (2026-10-12, 15:13) THE OWNER ACCEPTED v-ship's PICTURE IN THE GAME, AND ITS THREE PICTURE CUTS ARE THE PRODUCT'S NOW (CUTS.txt: "accepted";
# docs\reports\fg-3\sitting-2026-10-12b\BRIEF-ship.md). So EVERY VARIANT HERE IS BUILT ON THE EXACT LIBRARY'S CUTS (-Cuts exact,+<cut>;
# tools\ffx-own\cuts.py), not on the product's: each is the library it was before that day, byte for byte, for
# comparison ("env" is v-env alone, "search-half" that cut alone), and none moves when the product takes a picture
# cut. "ship" itself is kept as a FROZEN library below: the bytes the owner ran, under the name they ran by.
OUT = os.path.join(ROOT, "obj", "dlssiv-fg-var")
# (2026-10-12) THE FROZEN LIBRARIES: A VARIANT AS IT RAN IN THE GAME, BYTE FOR BYTE, whatever the product's cuts and the variant's
# own cut have become since. A library's bytes hold the digest of its cuts and its own file's name, so each is built
# from EXACTLY the cuts it had (the product's twelve of 2026-10-11 and its own), under the name it had, into a folder
# of another name; a cut whose file has moved on is taken from its frozen copy (tools\ffx-own\cuts.py, "frozen-as").
# Each is held to its size and SHA-256: a build that is not those bytes is said and fails.
#     (the folder, the variant's name in its file's name, the cuts exactly, bytes, SHA-256)
PRODUCT_1011 = ["fi-game-field-test-first", "fi-depth-test-first", "fi-depth-into-game-field", "fi-setup-by-clears", "fi-no-distortion-field", "of-filter-into-scale", "of-search-by-rows",
                "of-search-sums-by-minimum", "fi-inpainting-by-blocks", "of-reset-no-luma-clears", "fi-interpolation-same-vector-once", "of-search-window-by-gather"]
# (2026-10-12) "ship": THE CANDIDATE AS THE OWNER RAN IT IN THE GAME THAT AFTERNOON (F1C flw and flh, both PASS), and the anchor of the
# product's picture: the product is these sixteen cuts under the product's own file name, and nothing else differs
# between the two builds (build-ffx-own.ps1: the link's /OUT name). While this entry builds to its SHA-256 from the
# tree, the tree still makes the library whose picture was accepted.
# (2026-10-12, the leftovers of the picture; the main session's addition to BRIEF-ship.md, part 6) THE VARIANTS ON TOP OF THE PRODUCT: these, and
# these alone, are built on the product's cuts (-Cuts +<cut>), because they are judged against the product of the day.
# NEITHER OF THE TWO WAS A MEND (each cut's own head has its numbers; tools\test-fg-own\next.py made them): "next" is the field's
# ordering by the depth at the place, "next-creep" the farther creep. They are kept buildable so that nobody tries them a third time.
ON_PRODUCT = [
    ("next",                  ["v-field-depth-then"]),
    ("next-creep",            ["v-env-creep-far"]),
]
# (2026-10-14, docs\reports\fg-4\brief-2026-10-14\BRIEF-busy-cost.md, Part 1, G) THE GUIDES CAPPED AT QUALITY'S SIZE, ON THE PRODUCT: ready to be
# judged on the owner's frames at DLAA the day a capture at DLAA exists (the kit's F1C capa), against the product of that day
# (tools\test-fg-own\lib4.py dlaa). At Quality and below it must be the product's picture, bit for bit (lib4.py same).
ON_PRODUCT += [
    ("guide-cap-product",     ["v-guide-cap"]),
]
# (2026-10-25, docs\reports\fg-5\desk-2026-10-25-edge\FIX.md) THE BAND AT THE SCREEN'S EDGE, ON THE PRODUCT: where the current frame's sample is
# off the screen, its own pixel at the place by the lone sample's need of brightness gain (ffx-own\cuts\v-edge.cut). Judged against the
# product of the day on the owner's capture of 2026-10-25, then by his eye in the game with frame generation.
# (2026-10-25, evening) v-edge IS THE PRODUCT'S (CUTS.txt: "accepted", the owner's pick of the sitting of that evening), and "edge" is
# kept below as a FROZEN library: the bytes the owner ran as edgeB. v-edge2 is "out" (its leg edgeC: the edge not clean): no variant.
# (2026-10-25, round 8, E2) THE GUIDES' IMAGES AT THE CAP, ON THE PRODUCT: a memory cut that is not exact on the owner's DLAA capture
# (ffx-own\cuts\fi-guide-images-at-cap.cut), so a picture for the owner's eye at DLAA; at Quality and below it allocates nothing less.
ON_PRODUCT += [
    ("guide-images-at-cap",   ["fi-guide-images-at-cap"]),
]
# (2026-10-26, round 9's V2, docs\reports\fg-5\desk-2026-10-26-ghosts) THE TYRE'S FOOT, ON THE PRODUCT: the occlusion test's taps that the depth
# cannot separate are separated by the vector, where the thing at the tap moves unlike the pixel (ffx-own\cuts\v-hide-by-vector.cut). Judged
# on the owner's captures of that evening by the replay, then by his eye in the game with frame generation.
# (2026-10-27, evening) v-hide-by-vector IS THE PRODUCT'S with the three cuts of round 9's tyre desk (CUTS.txt: "accepted", the owner's blind
# A/B of that afternoon, tyreA): the variant "tyre" is kept below as a FROZEN library, the bytes he ran. The variants built to compare
# them (hide-by-vector, hide-reach, darker, tyre-copy) and the instruments on them (fields-hbv, flow-hbv, trust-hbv, hbv-no-trust,
# fields-tc, current-darker, previous-darker) left with it: each is now the product, or an instrument below on the product.
# (2026-10-27, round 9's tyre desk, docs\reports\fg-5\desk-2026-10-27-tyres) AN INSTRUMENT ON THE PRODUCT: v-env-show-need on the product of the day
# (every pixel painted with the two log gains v-env applied and the current sample's share), to read what v-env does at the tyre's foot.
ON_PRODUCT += [
    ("need-product",          ["v-env-show-need"]),
    ("fields-product",        ["v-show-fields"]),
    ("flow-product",          ["v-show-flow"]),
    ("no-trust",              ["v-env-no-trust"]),
    ("trust-product",         ["v-env-show-trust"]),
    ("current-product",       ["v-show-current"]),
    ("previous-product",      ["v-show-previous"]),
]
# (2026-10-28, the tyre's leftovers, docs\reports\fg-5\desk-2026-10-28-tyre-leftovers) AN INSTRUMENT ON THE PRODUCT: the occlusion pass's decisions
ON_PRODUCT += [
    ("hide-product",          ["v-show-hide"]),
]
# (2026-10-28, the same desk) THE TYRE'S TWO LEFTOVERS, ON THE PRODUCT: the thin dark line at the very foot (v-reach-nearer) and the light specks on the
# tyre's front side (v-ahead-before), each alone and together ("tyre2", for the owner's blind A/B against the product). OUT the same day: the
# owner's blind A/B saw no gain (CUTS.txt), so the four variants that built them are gone; the cut files stay as the record of what was tried.
VARIANTS += ON_PRODUCT
EXACT_1012 = PRODUCT_1011 + ["fi-inpainting-keeps-the-source"]
SHIP = ["v-search-half", "v-no-inpainting-pyramid", "v-env"]
# (2026-10-25) the product's 21 cuts of that day, in their order (RECORD-dlssiv-fg-cut.txt of 722058a4, digest 0efb6e71)
PRODUCT_1025 = EXACT_1012 + ["fi-position", "fi-store-apart", "v-search-half", "v-guide-cap", "v-no-inpainting-pyramid", "v-env", "v-env-position", "fi-cover-in-alpha"]
# (2026-10-27) the product's 23 cuts before the tyre fix (RECORD-dlssiv-fg-cut.txt of 2891d501, digest fdb39d16)
PRODUCT_1027 = PRODUCT_1025 + ["v-edge", "fi-one-shared-set"]
FROZEN = [
    ("ghost-1011", "ghost", PRODUCT_1011 + ["v-ghost"], 821760, "8b4c61f7fa63fb597e80ede1e299eeecdfe0ff28ba56d9d8bb2c8d065bc6d181"),
    ("env-1",      "env",   PRODUCT_1011 + ["v-env-1"], 811520, "dc50b61fbfa3e11ec663bbe0ed6393a18c0e87e584fbe83343379fa4acfa98c7"),
    ("ship",       "ship",  EXACT_1012 + SHIP,          809472, "bf4dbd78c83a1400fedfcfb4b2e5e3d52dc194037ec942f96e7f623fa6960a24"),
    # (2026-10-25, evening) "edge": v-edge ON THE PRODUCT OF 2026-10-25 (its 21 cuts and v-edge), AS THE OWNER RAN IT AS edgeB on pair
    # 93b3ef9b and picked it. The product took v-edge and the two exact memory cuts beside it; this is the anchor they are held to
    # (tools\test-fg-own\run.py --ours --pictures --pictures-of=<this file>: the product's pictures, pair by pair)
    ("edge",       "edge",  PRODUCT_1025 + ["v-edge"],  814592, "4786defe10e36a9223c39e8ef77182b1458632928a03736b12e3c052a14de8f7"),
    # (2026-10-27, evening) "tyre": round 9's tyre fix ON THE PRODUCT OF 2026-10-27 (its 23 cuts and the four), AS THE OWNER RAN IT AS tyreA
    # on pair 6200f252 and picked it blind (docs\reports\fg-5\desk-2026-10-27-tyres\RECORD-dlssiv-fg-v-tyre.txt). The product took the four;
    # this is the anchor it is held to (run.py --ours --new-pictures --pictures-of=<this file>: the product's pictures, pair by pair)
    ("tyre",       "tyre",  PRODUCT_1027 + ["v-hide-by-vector", "v-hide-reach", "v-env-darker", "v-lone-place"], 822272,
     "3206fcd8247f0b2c45b2b9ab376c4e8fa3a7405ae6ba92290ea05f29815a0f14"),
]


def path_of(name):
    for folder, variant, cs, size, sha in FROZEN:
        if name == folder: return os.path.join(OUT, folder, "dlssiv-fg-v-%s.dll" % variant)
    return os.path.join(OUT, name, "dlssiv-fg-v-%s.dll" % name)


def main():
    a = sys.argv[1:]
    state = dict(cuts.read_list())
    listed = sorted(n for n, s in state.items() if s in ("picture", "accepted"))
    # (2026-10-14: an "accepted" cut is held by THE PRODUCT's own build, which every variant ON_PRODUCT is built on: fi-cover-in-alpha, accepted on
    # 2026-10-13 and in no variant, made this check refuse every build from that day)
    held = sorted({c for n, cs in VARIANTS if n != "all" for c in cs} | {c for f in FROZEN for c in f[2] if state.get(c) in ("picture", "accepted")} | {n for n, s in state.items() if s == "accepted"})
    if listed != held:
        print("FAIL: CUTS.txt lists the picture cuts %s and this script builds %s" % (", ".join(listed), ", ".join(held))); return 1
    if a == ["--list"]:
        for name, cs in VARIANTS: print("%-22s %s" % (name, ", ".join(cs)))
        for folder, variant, cs, size, sha in FROZEN: print("%-22s FROZEN, as dlssiv-fg-v-%s.dll, %d bytes, SHA-256 %s: exactly %s" % (folder, variant, size, sha, ", ".join(cs)))
        return 0
    want = [v for v in VARIANTS if not a or v[0] in a]
    frozen = [f for f in FROZEN if f[0] in a]
    if a and len(want) + len(frozen) != len(a):
        print("FAIL: no variant named %s (--list)" % ", ".join(x for x in a if x not in [v[0] for v in VARIANTS] + [f[0] for f in FROZEN])); return 1
    bad = 0
    import hashlib
    for folder, variant, cs, size, sha in frozen:
        out = os.path.join(OUT, folder)
        r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Variant", variant,
                            "-Cuts", ",".join(cs), "-OutDir", out], capture_output=True, text=True, errors="replace", cwd=ROOT)
        dll = path_of(folder)
        got = hashlib.sha256(open(dll, "rb").read()).hexdigest() if r.returncode == 0 and os.path.isfile(dll) else None
        if got == sha and os.path.getsize(dll) == size:
            print("%-22s %d bytes, SHA-256 %s  AS IT RAN IN THE GAME, byte for byte  %s" % (folder, size, sha, dll))
        else:
            bad += 1
            print("%-22s NOT THE BYTES IT WAS (%s; wanted %d bytes, SHA-256 %s)" % (folder, "%d bytes, SHA-256 %s" % (os.path.getsize(dll), got) if got else "not built, exit code %d" % r.returncode, size, sha))
            for l in (r.stdout + r.stderr).splitlines()[-12:]: print("    " + l)
    for name, cs in want:
        out = os.path.join(OUT, name)
        r = subprocess.run(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Variant", name,
                            "-Cuts", ("" if (name, cs) in ON_PRODUCT else "exact,") + ",".join("+" + c for c in cs), "-OutDir", out], capture_output=True, text=True, errors="replace", cwd=ROOT)
        dll = path_of(name)
        ok = r.returncode == 0 and os.path.isfile(dll)
        if ok:
            print("%-22s %d bytes, SHA-256 %s  %s" % (name, os.path.getsize(dll), hashlib.sha256(open(dll, "rb").read()).hexdigest(), dll))
        else:
            bad += 1
            print("%-22s NOT BUILT (exit code %d):" % (name, r.returncode))
            for l in (r.stdout + r.stderr).splitlines()[-25:]: print("    " + l)
    n = len(want) + len(frozen)
    print("PASS: %d variant librar%s built under %s" % (n, "y" if n == 1 else "ies", OUT) if not bad else "FAIL: %d of %d not built" % (bad, n))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
