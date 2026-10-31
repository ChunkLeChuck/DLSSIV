#!/usr/bin/env python3
r"""THE OWN BUILD OF AMD'S FRAME GENERATION, PROVED WITH NO DEVICE (2026-10-06, stage 1a of
docs\reports\fg-3\own-build-2026-10-06\critic-plan.md, "THE REPRODUCTION"; held to a record since the review of the
same day, docs\reports\fg-3\own-build-2026-10-06\stage-1a-review).

    python tools\prove-ffx-own.py               about two minutes; the last line is "PASS: ..." or "FAIL: <the first failure>"
    python tools\prove-ffx-own.py --new-record  makes tools\ffx-own\RECORD.txt again from this working tree, ON PURPOSE,
                                                and prints every value it replaced. That run is never a PASS.

One command for everything that can be said about ffx-own-fg.dll before it meets a GPU. Each part has a CONTROL: the
same check on something made wrong on purpose, which must fail. A check that cannot fail proves nothing.

THE RECORD. tools\ffx-own\RECORD.txt names the library that was proven (size, SHA-256), the toolchain its bytes
belong to (cl, link, the Windows SDK), one digest of the 72 shader headers and one of the manifest's rows. Before it
existed this proof compared one build with another build of the same run and with nothing else, and the review showed
what that is worth: a library built with /fp:fast taken from the environment, with /O1 in an edited script, with
another dxc argument, or from a source file changed together with its manifest row, each ended in PASS. Each of the
four is now a control below and must be refused.

  0. WHAT THIS RUN READS IS WHAT IS COMMITTED. Nothing is uncommitted under external\ffx, ffx-own, tools\ffx-own, or in
     .gitattributes, build-ffx-own.ps1 and this file. And the compiler's four environment variables (CL, _CL_, LINK,
     _LINK_) are cleared for every build of the run; the run says so if one was set.
  1. THE SOURCE IS AMD'S. Every file in external\ffx\ffx_api_2.3.0 is the file the manifest lists by its git blob id,
     and the folder holds nothing else (tools\ffx-own\ffx_manifest.py, offline); and the manifest's rows are the
     record's, by one digest: the rows the review held to GitHub's listing of AMD's tag v2.3.0.
     AGAINST AMD ITSELF, WHEN IT CAN BE: if a saved copy of GitHub's listing of the tag's tree is at
     obj\ffx-own\tree-v2.3.0.json (or where DLSSIV_FFX_TREE says; ffx_manifest.py gives the address), every row must
     be in it with that path, git blob id and size, and a copy of the listing with one id changed must fail. If no
     listing is there the run says NOT CHECKED AGAINST AMD, in part 1 and again on its last line. It fetches nothing.
     Controls, in a temp copy: one byte changed in one file; one file with CRLF line endings; one stray file; one
     file removed (named, with the one cause nobody would guess: a working tree under a root too long for git); and one
     line of an AMD file changed TOGETHER with its manifest row (the manifest check passes on that, the record does not).
  2. THE HEADERS ARE THE SOURCE'S. The 72 committed shader headers equal a regeneration by the pinned dxc, byte for
     byte (tools\ffx-own\gen_shaders.py, which also checks every container: signed by dxil.dll, the variant's shader
     model, no 16-bit type, registers and names), and they are the record's, by one digest.
     Controls: one byte changed in one header; a dxc with one byte changed in each of its three files in turn (refused
     with one line, never started); one digit changed in a copy of AMD's shader source (the regeneration differs);
     "-O1" added to dxc's command line in a copy of the generator (the copy refuses its own command line); and the
     same with "-O1" also written into the copy's list of our own tokens (it runs, and its headers are not the record's).
  3. A FRESH CHECKOUT GIVES THE SAME BYTES. HEAD is checked out into a temp folder with core.autocrlf=true, and the
     manifest check and the 72 headers hold there: src\DLSS-IV\.gitattributes keeps git's line-ending conversion out.
     Control: a file outside that rule (external\ffx\VERSION.txt) did come out with CRLF, so the conversion was on.
  4. THE LIBRARY IS THE RECORD'S, FROM ANY FOLDER. build-ffx-own.ps1 in this tree: the toolchain it reports is the
     record's (another one fails here with a line that says so), and the library is the record's size and SHA-256.
     Again from a copy of its inputs in a temp folder with a longer path and a space in it, this time with
     _CL_=/fp:fast in the environment: the script says it cleared it and the library is byte-equal.
     Controls, each a library the record refuses with both hashes: a copy of the script that does not clear the
     environment, built with _CL_=/fp:fast; a copy with /O2 changed to /O1; a copy with one line of our glue changed.
     And a toolchain file naming another compiler is refused.
     THE OUTPUT FOLDER (the review's R3.1 and R3.2). Controls: somebody else's .dll and .cmd planted in the output
     folder are there, byte for byte, after a build; the /MD twin goes into a folder of its own and the /MT library
     beside it is untouched; a compile warning planted in a copy leaves NO ffx-own-fg.dll where one stood before;
     and a file named like AMD's in the output folder stops the script before it deletes or writes anything.
     And a temp folder whose path has a letter outside ASCII (the review's R3.5) is refused with a plain line that
     names DLSSIV_FFX_STAGE, before anything is touched.
     And of the library: 64-bit, exports AMD's five entry points and nothing else, imports no VC++ runtime DLL,
     carries no version resource, no signature and no PDB path, and no file in a build folder is named like AMD's.
  5. THE BUILD READS WHAT THE MANIFEST SAYS IT READS, BY ITS BYTES. The files of AMD's that cl and dxc opened are
     exactly the manifest's rows marked "own": nothing vendored for the build goes unread, nothing unlisted is read;
     and each was hashed where it was compiled and has the manifest's git blob id, so the files that were compiled
     are the files part 1 checked (the review's R1.3: only the names were compared before).
     Control: a copy of the inputs with one character changed in one of AMD's files builds, and its list is refused
     for that file.
     And the compiler may open nothing but the build's own copy, its own headers and the Windows SDK's. Control: a
     copy whose glue lacks pix3.h, with a pix3.h in a foreign folder that INCLUDE names: the build fails, names the
     file, and delivers no library.
  6. THE LIBRARY ANSWERS AS AMD'S DOES. ffx-own-probe.exe loads it by full path and asks for its versions with a null
     device (no Direct3D device, no window): one frame generation provider, the name and id external\ffx\VERSION.txt
     records for AMD's own library ("3.1.6", 0xF600000000C01006), no swapchain provider, no upscaler.
     Control: a library of ours that answers "3.1.5": the probe must fail on it.
  7. THE TWO TEST LIBRARIES CAN BE MADE (tools\ffx-own\build_alt.py: the controls the next stage's picture compare
     must fail on, one constant changed in the interpolation shader, one in optical flow's search). Each is built
     into a temp folder from an altered copy of the kit, is not the record's library, calls itself by its test name,
     exports the same five names, and differs from the product in exactly the two headers of its pass.
     Control: this tree, which is not altered, refuses -TestName and builds nothing: the unaltered source cannot come
     out under a test's name.
  8. THE SECOND LIBRARY (since the own build's stage 2, 2026-10-10): dlssiv-fg.dll, AMD's frame interpolation, optical
     flow and backend behind THE MOD'S OWN INTERFACE (ffx-own\glue\dlssiv_fg.h; build-ffx-own.ps1 -Target dlssiv-fg),
     held to a record of its own, tools\ffx-own\RECORD-dlssiv-fg.txt, as the first is held to RECORD.txt.
     THE OBJECTS OF AMD'S FILES IN IT ARE THE REFERENCE LIBRARY'S: the build compiles once, links ffx-own-fg.dll from
     those objects first and writes its SHA-256 beside the second library (objects.txt); that sibling must be the
     FIRST record's library. It is linked from eleven objects (nine of AMD's twelve files, the swapchain's two stubs,
     the interface), and its map names no object of AMD's ffx-api core, query fallback or provider layer.
     The same bytes from a second folder (with _CL_=/fp:fast in the environment, cleared); its six files there and
     nothing else. It read the same files of AMD's as the first build, each with the manifest's git blob id.
     Of the library: 64-bit, calls itself dlssiv-fg.dll, exports the interface's seven functions and none of AMD's
     five, imports no VC++ runtime DLL, carries no resource, no signature and no PDB path, names no library of AMD's.
     AND IT ANSWERS WITH NO DEVICE (tools\ffx-own\probe_fg.py, in a process of its own): interface 1, AMD's "3.1.6"
     and id 0xF600000000C01006; a description of another size, a null create and a destroy of nothing are refused
     with the interface's own codes.
     Controls: one line of our interface changed in a copy: a library the second record refuses, whose sibling is
     still the first record's (AMD's objects did not change). One character of an AMD file changed in a copy: its
     sibling is NOT the first record's. -DynamicCrt with this target is refused and builds nothing. The probe on
     stage 1's library fails (it has none of the seven functions). And the two altered test libraries of part 7 are
     made a second time from the same altered trees behind this interface (dlssiv-fg-alt-fi.dll, dlssiv-fg-alt-of.dll):
     neither is a recorded library, each calls itself by its test name and exports the same seven names.
     python tools\prove-ffx-own.py --new-record-fg makes that second record again, ON PURPOSE, with everything of the
     first library held to RECORD.txt as in every run. That run is never a PASS.
     SINCE THE OWN BUILD'S STAGE 3 (2026-10-10) THAT LIBRARY IS THE REFERENCE: the same build delivers it, with no cut,
     as dlssiv-fg-ref.dll into obj\dlssiv-fg-ref (its bytes are stage 2's and still call themselves dlssiv-fg.dll),
     and everything above is said of that file. RECORD-dlssiv-fg.txt is unchanged.
  9. THE PRODUCT (since stage 3): dlssiv-fg.dll WITH THE CUTS THAT ARE IN (ffx-own\cuts\CUTS.txt; tools\ffx-own\cuts.py),
     built by the same run of the same script right after the reference, held to a record of its own,
     tools\ffx-own\RECORD-dlssiv-fg-cut.txt: size, SHA-256, the toolchain, and four keys more: how many cuts are in,
     one digest of them, how many shader headers they change and one digest of those.
     ITS OWN LIST NAMES THE REFERENCE IT WAS BUILT BESIDE (objects.txt): the first record's library as the sibling and
     stage 2's record's as the reference, so the product is that source, that compiler and those options plus the
     cuts cuts.txt names, and nothing else. cuts.txt's count and digest are the list's. THE HEADERS OF THE CUTS
     (ffx-own\shaders-cut) are what the cut source compiles to: all 18 passes are compiled again from a copy the cuts
     were applied to, the headers that differ from ffx-own\shaders are the committed ones byte for byte, and the
     folder holds nothing else. The same bytes from a second folder. With no cut in, the product is the reference byte
     for byte; with one in it is not, and says "stage 3" and how many cuts with no device.
     AMD'S VENDORED FILES ARE STILL AMD'S after every build of the run (the manifest check again: an edit is made in
     the build's temp copy and nowhere else).
     Controls: -Cuts none gives the reference's bytes (every cut has a switch), and, with a cut in, the product less
     its first cut is another library; -Cuts without a folder of its own is refused and builds nothing; a cut whose
     old text is not in AMD's file (one word changed in a copy of the one-pixel control) stops the build with a line
     that names the cut and the file, and delivers no library; and the judge's control, dlssiv-fg-px.dll (the product's
     cuts and one pixel changed), is made, is not a recorded library, and lists the control among its cuts.
     python tools\prove-ffx-own.py --new-record-cut makes the product's record again, ON PURPOSE, with the first
     library and the reference held to their records as in every run. That run is never a PASS.
     SINCE 2026-10-12 PART 9 IS SAID OF THE EXACT LIBRARY, AND THE PRODUCT HAS A PART OF ITS OWN. The owner accepted in the
     game a library with three cuts that change the picture, and it is the product; so "the product equals the
     reference" no longer holds and no longer proves that a cut is exact. What does:
     THE EXACT LIBRARY, dlssiv-fg-exact.dll: the cuts the list marks "in" and no picture cut, built by the same run
     between the reference and the product (obj\dlssiv-fg-exact), the bytes the product had until that day, held to
     a record of its own, tools\ffx-own\RECORD-dlssiv-fg-exact.txt (whose "library" is that file name, never
     dlssiv-fg.dll). Everything part 9 says above is said of it, and the judge holds IT to the reference and to AMD's.
     Its one-pixel control and the two altered libraries of part 7 are built on ITS cuts.
     python tools\prove-ffx-own.py --new-record-exact makes its record, ON PURPOSE; the product is left out of that run.
 10. THE PRODUCT, dlssiv-fg.dll: the exact library's cuts and, applied on top of them to the very copy the exact library
     was compiled from, the picture cuts the list marks "accepted". Its own list names the sibling, the reference AND
     the exact library (the file part 9 held); its cuts are the exact ones in their order and then the accepted ones;
     the headers its picture cuts change (ffx-own\shaders-picture: the ones that differ from the exact library's) are
     what the cut source compiles to; it is its record's (RECORD-dlssiv-fg-cut.txt, as before), the same bytes from a
     second folder; with no accepted cut it IS the exact library byte for byte. And THE PIN OF ITS PICTURES
     (tools\ffx-own\PICTURES-dlssiv-fg.txt) is there and was made for exactly these picture cuts, by their digest; the
     pictures themselves are held on the card (tools\test-fg-own\run.py --ours).
     Controls: -Cuts exact gives the exact library's bytes (every picture cut has a switch); the product less its first
     picture cut is another library than both; A PICTURE CUT THAT NOBODY ACCEPTED IS STILL REFUSED IN THE PRODUCT (-Cuts
     +<one> builds nothing outside a variant); the same cut marked "accepted" in a copy of the list by nobody's word
     gives a digest of cuts that is not the record's; an exact cut written behind an accepted one in a copy of the list
     is refused with a line that names it.

WHAT IT DOES NOT PROVE, ON PURPOSE. That our shaders are the ones inside AMD's signed library, and that the library
makes AMD's picture: no AMD binary is read here (the project does not look inside them), and nothing runs on a GPU.
The next stage proves both at once, by the generated picture of ours against AMD's on a device.

It writes nothing in the repository but the ignored build folder obj\ffx-own, where it leaves the library it proved
(and, with --new-record, tools\ffx-own\RECORD.txt). Everything runs at BelowNormal priority. It is not part of
tools\prove-all.py yet: the main session wires it in.
"""
import ctypes, hashlib, os, re, shutil, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools", "ffx-own"))
import build_alt
import cuts as C
import ffx_manifest as M
import ffx_record as R
import gen_shaders as G
import pe_read

PS = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File"]
NO_WINDOW = 0x08000000
FIVE = ["ffxConfigure", "ffxCreateContext", "ffxDestroyContext", "ffxDispatch", "ffxQuery"]
SEVEN = ["dlssivFgCreate", "dlssivFgDestroy", "dlssivFgInfo", "dlssivFgInterpolate", "dlssivFgOpticalFlow", "dlssivFgPrepare", "dlssivFgStamps"]   # the second library's own names
ELEVEN = ["ffx_assert.obj", "ffx_message.obj", "ffx_object_management.obj", "ffx_dx12.obj", "ffx_backends_dx12.obj", "ffx_frameinterpolation.obj", "ffx_frameinterpolation_shaderblobs.obj",
          "ffx_opticalflow.obj", "ffx_opticalflow_shaderblobs.obj", "own_no_swapchain.obj", "dlssiv_fg.obj"]                                           # what it is linked from
NOT_LINKED = ["ffx_api.obj", "ffx_query_fallback.obj", "ffx_provider_fsr3framegeneration.obj", "own_providers.obj"]                                    # and what it is not
BUILD_INPUTS = ["build-ffx-own.ps1", os.path.join("external", "ffx", "ffx_api_2.3.0"), os.path.join("ffx-own", "glue"), os.path.join("ffx-own", "shaders"),
                os.path.join("ffx-own", "cuts"), os.path.join("ffx-own", "shaders-cut"), os.path.join("ffx-own", "shaders-picture"),                   # the cuts, and the headers of the exact ones and of the product's picture cuts (a folder that is not there while no such cut changes a shader)
                os.path.join("tools", "ffx-own", "cuts.py"), os.path.join("tools", "ffx-own", "gen_shaders.py"), os.path.join("tools", "ffx-own", "dxbc.py")]
SIX_REF = ["build.log", "dlssiv-fg-ref.dll", "dlssiv-fg-ref.map", "objects.txt", "reads.txt", "toolchain.txt"]                                       # what the build leaves in the reference's folder
SEVEN_FILES = ["build.log", "cuts.txt", "dlssiv-fg.dll", "dlssiv-fg.map", "objects.txt", "reads.txt", "toolchain.txt"]                               # and in the product's
SEVEN_EXACT = ["build.log", "cuts.txt", "dlssiv-fg-exact.dll", "dlssiv-fg-exact.map", "objects.txt", "reads.txt", "toolchain.txt"]                   # and in the exact library's
HELD = ["external/ffx", "ffx-own", ".gitattributes", "build-ffx-own.ps1", "tools/ffx-own", "tools/prove-ffx-own.py"]     # what part 0 wants committed
RUNTIME = re.compile(r"^(msvcp|vcruntime|ucrtbase|api-ms-win-crt-|concrt|vcomp|msvcr)", re.I)
CL_VARS = ("CL", "_CL_", "LINK", "_LINK_")
ENV = {k: v for k, v in os.environ.items() if k.upper() not in CL_VARS}      # every build of this run gets this one
WAS_SET = {k: v for k, v in os.environ.items() if k.upper() in CL_VARS}
NEW = False                                                                # --new-record: nothing is compared with the record
NEW_FG = False                                                             # --new-record-fg: the SECOND library's record is made; the first's is held as ever
NEW_CUT = False                                                            # --new-record-cut: THE PRODUCT'S record is made; the first's, the reference's and the exact library's are held as ever
NEW_EXACT = False                                                          # --new-record-exact: THE EXACT LIBRARY'S record is made; the first's and the reference's are held as ever, the product is left out


class Failed(Exception):
    pass


def ok(text):
    print("  ok    " + text); sys.stdout.flush()


def need(cond, text, why=""):
    """one line of the proof: said when it holds, the end of the run when it does not"""
    if not cond: raise Failed(text + ((": " + why) if why else ""))
    ok(text)


def control(failed, text, why=""):
    """a control holds when the check FAILED on the thing made wrong"""
    if not failed: raise Failed("CONTROL: " + text + " was NOT caught" + ((": " + why) if why else ""))
    ok("control: " + text + ((" (" + why + ")") if why else ""))


def run(cmd, cwd=None, env=None):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors="replace", creationflags=NO_WINDOW, env=env or ENV)
    return r.returncode, (r.stdout or "") + (r.stderr or "")


def sha(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def remove(folder):
    """a temp folder and everything in it, read-only files too (git writes its objects read-only)"""
    def again(fn, path, exc):
        try: os.chmod(path, 0o700); fn(path)
        except OSError: pass
    try: shutil.rmtree(folder, onexc=again)
    except TypeError: shutil.rmtree(folder, onerror=again)          # a Python older than 3.12


def flip(path, at=None):
    """change one byte of a file; -> its offset"""
    d = bytearray(open(path, "rb").read())
    i = len(d) // 2 if at is None else at
    d[i] ^= 1
    open(path, "wb").write(d)
    return i


def swap(path, old, new):
    """replace the one occurrence of old in a file; Failed when it is not there exactly once"""
    d = open(path, "rb").read()
    if d.count(old) != 1: raise Failed("the control's text %r is in %s %d time(s), not once" % (old[:60], os.path.basename(path), d.count(old)))
    open(path, "wb").write(d.replace(old, new))
    return d


# ---- 0 ------------------------------------------------------------------------------------------------------------------
def part_tree():
    print("0. what this run reads is what is committed")
    rc, top = run(["git", "-C", ROOT, "rev-parse", "--show-toplevel"])
    need(rc == 0, "this is a git working tree", top.strip())
    top = top.strip()
    rel = os.path.relpath(ROOT, top).replace(os.sep, "/")
    rc, dirty = run(["git", "-C", top, "status", "--porcelain", "--"] + [rel + "/" + p for p in HELD])
    clean = rc == 0 and not dirty.strip()
    what = "nothing is uncommitted under external\\ffx, ffx-own, tools\\ffx-own, or in .gitattributes, build-ffx-own.ps1 and tools\\prove-ffx-own.py"
    if (NEW or NEW_FG or NEW_CUT or NEW_EXACT) and not clean: ok("NEW RECORD: the working tree has uncommitted changes (%d path(s)); the record is made from it as it is, and part 3 is left out" % len(dirty.strip().splitlines()))
    else: need(clean, what, "; ".join(dirty.strip().splitlines()[:6]))
    if WAS_SET: ok("this environment had %s set: cleared for every build of this run (cl and link read them as options, silently)" % ", ".join("%s=%s" % kv for kv in sorted(WAS_SET.items())))
    else: ok("none of CL, _CL_, LINK, _LINK_ is set in this environment (they are cleared for every build of the run anyway)")
    return top, rel, clean


# ---- 1 ------------------------------------------------------------------------------------------------------------------
def part_source(tmp, rec):
    print("1. the source is AMD's")
    bad, n, total = M.check()
    need(not bad, "the manifest: %d files, %d bytes, each the file the manifest lists by its git blob id, and nothing else in the folder" % (n, total), "; ".join(bad[:3]))
    head, rows = M.read_manifest(os.path.join(M.FFX, M.MANIFEST))
    need(head["tag"] == "v2.3.0" and head["commit"] == "60f4ea81909200d8542eca14dccb2628b763a9a3", "the manifest names tag v2.3.0, commit 60f4ea81")
    count, digest = R.manifest_digest(rows)
    if NEW: ok("NEW RECORD: the manifest's %d rows, digest %s" % (count, digest))
    else: need((str(count), digest) == (rec["manifest-rows"], rec["manifest-sha256"]), "the manifest's %d rows are the record's, by one digest (%s...): the rows the review held to GitHub's listing of AMD's tag" % (count, digest[:16]),
               "the manifest has %d rows with digest %s, the record says %s rows with digest %s" % (count, digest, rec["manifest-rows"], rec["manifest-sha256"]))
    listing = os.environ.get("DLSSIV_FFX_TREE") or os.path.join(ROOT, "obj", "ffx-own", "tree-v2.3.0.json")
    amd = os.path.isfile(listing)
    if amd:
        import json
        bad, n, files = M.against_listing(listing)
        need(not bad, "AGAINST AMD: GitHub's listing of the tag's tree (%s, %d files) has all %d rows, each with that path, git blob id and size" % (listing, files, n), "; ".join(bad[:3]))
        j = json.load(open(listing, encoding="utf-8-sig"))
        victim = next(e for e in j["tree"] if e.get("path") == rows[0][3])
        victim["sha"] = victim["sha"][:-1] + ("0" if victim["sha"][-1] != "0" else "1")
        json.dump(j, open(os.path.join(tmp, "listing.json"), "w", encoding="utf-8"))
        bad = M.against_listing(os.path.join(tmp, "listing.json"))[0]
        control(len(bad) == 1 and rows[0][3] in bad[0] and "GitHub's listing says id" in bad[0], "a copy of that listing with one id changed", "named, with both ids")
    else:
        ok("NOT CHECKED AGAINST AMD in this run: no saved copy of GitHub's listing of the tag's tree is at %s. The rows are held to the record's digest, which the review held to that listing "
           "on 2026-10-06. To check here: save the answer of %s there (or name the file in DLSSIV_FFX_TREE)" % (listing, M.LISTING_URL % head["commit"]))
    copy = os.path.join(tmp, "ffx")
    os.makedirs(copy); shutil.copy2(os.path.join(M.FFX, M.MANIFEST), copy); shutil.copytree(os.path.join(M.FFX, M.FOLDER), os.path.join(copy, M.FOLDER))
    need(not M.check(copy)[0], "the same check passes on an untouched temp copy (the controls start from it)")
    victim = os.path.join(copy, M.FOLDER, "framegeneration", "fsr3", "internal", "ffx_opticalflow.cpp")
    keep = open(victim, "rb").read()
    flip(victim)
    bad = M.check(copy)[0]
    control(len(bad) == 1 and "ffx_opticalflow.cpp is not AMD's file" in bad[0], "one byte changed in ffx_opticalflow.cpp", "named, with both ids")
    open(victim, "wb").write(keep.replace(b"\n", b"\r\n"))
    bad = M.check(copy)[0]
    control(len(bad) == 1 and "CRLF line endings" in bad[0], "the same file with CRLF line endings", "named as that case")
    open(victim, "wb").write(keep)
    stray = os.path.join(copy, M.FOLDER, "api", "internal", "not_amds.h")
    open(stray, "w").write("// not AMD's\n")
    bad = M.check(copy)[0]
    control(len(bad) == 1 and "not in the manifest" in bad[0], "a file in the folder that the manifest does not list")
    os.remove(stray)
    os.remove(victim)
    bad = M.check(copy)[0]
    control(len(bad) == 1 and "ffx_opticalflow.cpp is listed and is not there" in bad[0] and "core.longpaths" in bad[0], "a listed file removed", "named, with the line about a working tree under a root too long for git")
    open(victim, "wb").write(keep)
    # the file AND its row: what the manifest check cannot see by itself
    liar = os.path.join(copy, M.FOLDER, "framegeneration", "fsr3", "internal", "ffx_frameinterpolation.cpp")
    was = swap(liar, b"cosf(0.5f * params->cameraFovAngleVertical)", b"cosf(0.6f * params->cameraFovAngleVertical)")
    mp = os.path.join(copy, M.MANIFEST)
    swap(mp, M.blob_id(was).encode("ascii"), M.blob_id(open(liar, "rb").read()).encode("ascii"))
    lied = R.manifest_digest(M.read_manifest(mp)[1])[1]
    control(not M.check(copy)[0] and lied != digest, "one line of AMD's ffx_frameinterpolation.cpp changed TOGETHER with its manifest row: the manifest check passes on it, the record's digest does not",
            "%s... is not %s..." % (lied[:16], digest[:16]))
    return rows, count, digest, amd


# ---- 3 ------------------------------------------------------------------------------------------------------------------
def part_checkout(tmp, files, top, rel, clean):
    print("3. a fresh checkout gives the same bytes")
    if not clean:
        ok("NEW RECORD: left out, the working tree is not HEAD. The run that follows the commit does it")
        return
    paths = [rel + "/external/ffx", rel + "/ffx-own", rel + "/.gitattributes"]
    rc, head = run(["git", "-C", top, "rev-parse", "HEAD"])
    rc2, common = run(["git", "-C", top, "rev-parse", "--path-format=absolute", "--git-common-dir"])
    need(rc == 0 and rc2 == 0, "HEAD and the repository's object store are found")
    # A NEW, EMPTY REPOSITORY THAT BORROWS THIS ONE'S OBJECTS (an "alternates" line) and checks out three paths of HEAD:
    # a real git checkout under core.autocrlf=true, without copying the repository (a depth-1 fetch of it is 248 MB) and
    # without writing anything into it (a second worktree would).
    clone = os.path.join(tmp, "fresh")
    rc, out = run(["git", "init", "-q", clone])
    if rc != 0: raise Failed("the fresh checkout: git init said: %s" % out.strip()[:400])
    with open(os.path.join(clone, ".git", "objects", "info", "alternates"), "w", newline="\n") as f: f.write(os.path.join(common.strip(), "objects").replace("\\", "/") + "\n")
    steps = [["git", "-C", clone, "config", "core.autocrlf", "true"],
             ["git", "-C", clone, "sparse-checkout", "set", "--no-cone"] + ["/" + p for p in paths],
             ["git", "-C", clone, "checkout", "-q", "--detach", head.strip()]]
    for s in steps:
        rc, out = run(s)
        if rc != 0: raise Failed("the fresh checkout: %s said: %s" % (" ".join(s[:4]), out.strip()[:400]))
    rc, got = run(["git", "-C", clone, "rev-parse", "HEAD"])
    need(got.strip() == head.strip(), "HEAD (%s) checked out into a temp folder with core.autocrlf=true" % head.strip()[:8])
    base = os.path.join(clone, rel.replace("/", os.sep))
    bad, n, total = M.check(os.path.join(base, "external", "ffx"))
    need(not bad, "there the manifest check passes: %d files, %d bytes, AMD's LF line endings as committed" % (n, total), "; ".join(bad[:3]))
    bad = G.compare(files, os.path.join(base, "ffx-own", "shaders"))
    need(not bad, "and its 72 headers are byte-equal to the regeneration of part 2", "; ".join(bad[:3]))
    v = open(os.path.join(base, "external", "ffx", "VERSION.txt"), "rb").read()
    control(v.count(b"\r\n") > 0 and v.count(b"\r\n") == v.count(b"\n"), "a file outside the rule, external\\ffx\\VERSION.txt, did come out with CRLF: the conversion was on",
            "%d line endings, all CRLF" % v.count(b"\n"))


# ---- 2 ------------------------------------------------------------------------------------------------------------------
def generator_copy(tmp, name, edits):
    """a copy of the generator with some text replaced, run on the real kit; -> (exit code, what it said, its headers' folder)"""
    folder = os.path.join(tmp, name); os.makedirs(folder)
    for f in ("gen_shaders.py", "dxbc.py"): shutil.copy2(os.path.join(ROOT, "tools", "ffx-own", f), folder)
    for old, new in edits: swap(os.path.join(folder, "gen_shaders.py"), old, new)
    out = os.path.join(folder, "headers")
    rc, said = run([sys.executable, os.path.join(folder, "gen_shaders.py"), "--kit", G.KIT, "--out", out])
    return rc, said, out


def part_headers(tmp, rec):
    print("2. the headers are the source's")
    try: files, facts = G.generate()
    except G.Refused as e: raise Failed("the regeneration: " + str(e))
    ok(G.summary(facts, files))
    need(facts["base"] == 23 and facts["wave64"] == 23 and len(files) == 72, "every permutation AMD's scripts list is there: 23 containers cs_6_2 and 23 cs_6_6 (AMD's wave64 arguments), 72 headers")
    need(facts["wavesize"] == 5, "[WaveSize(64)] is on the entry of 5 of the 23 cs_6_6 containers, read from each container: the five optical flow passes whose source uses AMD's macro, and no other",
         "%d carry it" % facts["wavesize"])
    bad = G.compare(files, G.HEADERS)
    need(not bad, "the 72 committed headers equal the regeneration, byte for byte", "; ".join(bad[:3]))
    count, digest = R.headers_digest(G.HEADERS)
    if NEW: ok("NEW RECORD: the %d headers, digest %s" % (count, digest))
    else:
        need((str(count), digest) == (rec["headers"], rec["headers-sha256"]), "the %d committed headers are the record's, by one digest (%s...)" % (count, digest[:16]),
             "they are %d with digest %s, the record says %s with digest %s" % (count, digest, rec["headers"], rec["headers-sha256"]))
        need(G.PIN["version"] == rec["dxc"], "the generator's pinned dxc is the record's (%s)" % rec["dxc"], "the generator pins %s" % G.PIN["version"])
    copy = os.path.join(tmp, "headers"); shutil.copytree(G.HEADERS, copy)
    victim = os.path.join(copy, "ffx_frameinterpolation_pass_permutations.h")
    d = open(victim, "rb").read(); i = d.index(b"_data[] = {") + 40
    open(victim, "wb").write(d[:i] + (b"7" if d[i:i + 1] != b"7" else b"8") + d[i + 1:])
    bad = G.compare(files, copy)
    control(len(bad) == 1 and "ffx_frameinterpolation_pass_permutations.h is not what the shader compiles to" in bad[0], "one byte changed in one header", "named, with the line")
    control(R.headers_digest(copy)[1] != digest, "and that copy's digest is not the record's")
    real = facts["dxc"]
    for k, name in enumerate(("dxc.exe", "dxcompiler.dll", "dxil.dll")):
        w = os.path.join(tmp, "dxc-%d" % k); os.makedirs(w)           # the folder does not carry the file's name: the refusal must
        for f in ("dxc.exe", "dxcompiler.dll", "dxil.dll"): shutil.copy2(os.path.join(os.path.dirname(real), f), w)
        flip(os.path.join(w, name))
        os.environ["DXC_EXE"] = os.path.join(w, "dxc.exe")
        try:
            G.generate(); said = None
        except G.Refused as e: said = str(e)
        finally: del os.environ["DXC_EXE"]
        control(said is not None and said.startswith("REFUSED: ") and "\n" not in said and ("is not the %s of dxc" % name) in said, "a dxc whose %s differs by one byte is refused with one line that names that file, and is not started" % name)
        shutil.rmtree(w, ignore_errors=True)
    kit = os.path.join(tmp, "kit"); shutil.copytree(G.KIT, kit)
    swap(os.path.join(kit, "framegeneration", "fsr3", "include", "gpu", "opticalflow", "ffx_opticalflow_compute_optical_flow_v5.h"), b"(iSearchId.y & 0x7) == 0", b"(iSearchId.y & 0x3) == 0")
    name = "ffx_opticalflow_compute_optical_flow_advanced_pass_v5"
    try: changed, _ = G.generate(kit=kit, only=[name])
    except G.Refused as e: raise Failed("the control's regeneration: " + str(e))
    differs = [f for f in changed if "16bit" not in f and changed[f] != files[f]]
    control(sorted(differs) == [name + "_permutations.h", name + "_wave64_permutations.h"], "one digit changed in a copy of AMD's optical flow search: both of that pass's headers come out different")
    # dxc's command line (the review's R1.5): an argument more, first where the line is put together, then also in the list of our own tokens
    line = b'return [dxc, "-nologo"] + amd_args(group, variant, key)'
    rc, said, out = generator_copy(tmp, "gen-a", [(line, line.replace(b'"-nologo"]', b'"-nologo", "-O1"]'))])
    control(rc == 1 and "our dxc arguments are not the ones in AMD's scripts" in said and "-O1" in said and not os.path.isdir(out),
            "\"-O1\" added to dxc's command line in a copy of the generator: the copy refuses its own command line and compiles nothing")
    rc, said, out = generator_copy(tmp, "gen-b", [(line, line.replace(b'"-nologo"]', b'"-nologo", "-O1"]')), (b'OURS_FLAGS = ("-nologo",', b'OURS_FLAGS = ("-O1", "-nologo",')])
    need(rc == 0 and os.path.isdir(out), "(the same copy with \"-O1\" also written into its list of our own tokens runs: the token check cannot see an edit of its own list)", said.strip()[-300:])
    n2, d2 = R.headers_digest(out)
    differ = sum(1 for f in files if open(os.path.join(out, f), "rb").read() != files[f].encode("ascii"))
    control(n2 == 72 and d2 != digest and differ > 0, "but its headers are not the record's: %d of 72 differ, digest %s..." % (differ, d2[:16]))
    return files, facts, digest


# ---- 4 ------------------------------------------------------------------------------------------------------------------
def build(root, env=None, twin=False):
    """-> (the library or None, what the script said, its exit code)"""
    rc, out = run(PS + [os.path.join(root, "build-ffx-own.ps1")] + (["-DynamicCrt"] if twin else []), env=env)
    out = re.sub(r"\s+", " ", out)          # PowerShell breaks a long error message into lines at the console's width
    dll = os.path.join(root, "obj", "ffx-own-md", "ffx-own-fg-md.dll") if twin else os.path.join(root, "obj", "ffx-own", "ffx-own-fg.dll")
    return (dll if os.path.isfile(dll) else None), out, rc


def listing(folder):
    """{name: SHA-256} of a folder's files"""
    return {f: sha(os.path.join(folder, f)) for f in sorted(os.listdir(folder)) if os.path.isfile(os.path.join(folder, f))}


def built(root, env=None):
    dll, out, rc = build(root, env)
    if rc != 0 or dll is None or re.search(r"\bwarning\s+(C|LNK|D)\d+", out): raise Failed("build-ffx-own.ps1 in %s: exit code %d: %s" % (root, rc, out.strip()[-700:]))
    return dll, out


def stage(dst):
    for rel in BUILD_INPUTS:
        s = os.path.join(ROOT, rel); d = os.path.join(dst, rel)
        if not os.path.exists(s) and rel.endswith(("shaders-cut", "shaders-picture")): continue      # no such cut changes a shader: the folder is not there
        if os.path.isdir(s): shutil.copytree(s, d)
        else: os.makedirs(os.path.dirname(d), exist_ok=True); shutil.copy2(s, d)


def part_build(tmp, rec):
    print("4. the library is the record's, from any folder")
    t0 = time.time()
    here, said = built(ROOT)
    size = os.path.getsize(here); h = sha(here)
    tool = R.read_toolchain(os.path.join(os.path.dirname(here), "toolchain.txt"))
    need(all(tool.get(k) for k in R.TOOLCHAIN), "built in this tree with cl %s, link %s, Windows SDK %s: ffx-own-fg.dll, %d bytes, SHA-256 %s" % (tool.get("cl"), tool.get("link"), tool.get("windows-sdk"), size, h))
    ref = dict(rec) if rec else {}
    if NEW: ref.update({"size": str(size), "sha256": h, "cl": tool["cl"], "link": tool["link"], "windows-sdk": tool["windows-sdk"]}); ok("NEW RECORD: that library and that toolchain")
    else:
        why = R.toolchain_refusal(tool, rec)
        need(not why, "the toolchain is the record's", why)
        why = R.library_refusal(here, rec)
        need(not why, "the library is the record's: %s bytes, SHA-256 %s" % (rec["size"], rec["sha256"]), why + " (the toolchain is the record's, so an input or an option of the build is not)")
    other_tool = dict(tool, cl="19.99.0")
    why = R.toolchain_refusal(other_tool, ref)
    control(why.startswith("ANOTHER TOOLCHAIN: ") and "cl 19.99.0" in why and ("cl %s" % ref["cl"]) in why and "--new-record" in why, "a toolchain file that names another compiler is refused with one line that says both and how a new record is made")
    second = os.path.join(tmp, "a second folder, with a longer path and a space", "DLSS-IV")
    stage(second)
    out2 = os.path.join(second, "obj", "ffx-own"); os.makedirs(out2)
    foreign = {"somebody-elses.dll": b"not a library: a marker of the proof\r\n", "a-launcher.cmd": b"@echo a marker of the proof\r\n", "notes.map": b"a marker\r\n"}
    for f, data in foreign.items(): open(os.path.join(out2, f), "wb").write(data)
    env = dict(ENV, _CL_="/fp:fast")
    there, said = built(second, env)
    need(os.path.normcase(os.path.dirname(here)) != os.path.normcase(os.path.dirname(there)) and os.path.getmtime(there) >= t0 and os.path.getmtime(here) >= t0,
         "built again from a copy of its inputs in %s, with _CL_=/fp:fast in the environment" % second)
    need("_CL_ was set in the environment ('/fp:fast'): cleared" in said, "the script said that _CL_ was set and that it cleared it")
    need(open(here, "rb").read() == open(there, "rb").read(), "the two libraries are byte-equal", "this tree %s, the copy %s" % (h, sha(there)))
    gone = [f for f, data in foreign.items() if not os.path.isfile(os.path.join(out2, f)) or open(os.path.join(out2, f), "rb").read() != data]
    control(not gone, "somebody else's .dll, .cmd and .map planted in that output folder are there after the build, byte for byte: the script removes only files of its own names", ", ".join(sorted(foreign)))
    need(sorted(os.listdir(out2)) == sorted(list(foreign) + ["build.log", "ffx-own-fg.dll", "ffx-own-fg.map", "reads.txt", "toolchain.txt"]), "and the build left its five files there and nothing else: no object, no response file",
         ", ".join(sorted(os.listdir(out2))))
    twin, said, rc = build(second, twin=True)
    need(rc == 0 and twin is not None and os.path.dirname(twin) != out2, "the /MD twin builds into a folder of its own (obj\\ffx-own-md): ffx-own-fg-md.dll, %d bytes" % (os.path.getsize(twin) if twin else 0), said.strip()[-400:])
    need(sha(there) == h and bool(R.library_refusal(twin, ref)) and any(RUNTIME.match(d) for d in pe_read.PE(twin).imports()),
         "the /MT library beside it is untouched, and the twin is what it says: it imports the VC++ runtime and is not the record's library")
    odd = os.path.join(tmp, "st\u00e9"); os.makedirs(odd)           # a folder whose name ends in e with an acute accent
    before = listing(out2)
    dll, said, rc = build(second, dict(ENV, DLSSIV_FFX_STAGE=odd))
    control(rc != 0 and "has a character outside ASCII" in said and "DLSSIV_FFX_STAGE" in said and "Nothing was built and nothing was touched" in said and listing(out2) == before and not os.listdir(odd),
            "a temp folder with a letter outside ASCII in its path is refused with a plain line that names DLSSIV_FFX_STAGE, and the output folder is as it was, its library included")
    # the controls: three libraries that are not the record's, each from a third copy of the inputs with one thing changed
    third = os.path.join(tmp, "third", "DLSS-IV")
    stage(third)
    script = os.path.join(third, "build-ffx-own.ps1")

    def refused(what):
        dll, _ = built(third, env if "fp:fast" in what else None)
        why = R.library_refusal(dll, ref)
        control(bool(why) and sha(dll) in why and ref["sha256"] in why, what + ": a library the record refuses, with both hashes", "%d bytes, SHA-256 %s..." % (os.path.getsize(dll), sha(dll)[:16]))

    keep = open(script, "rb").read()
    d = keep
    for v in CL_VARS:
        d, n = re.subn(("    'set %s=',\r?\n" % v).encode("ascii"), b"", d)
        if n != 1: raise Failed("the control's line 'set %s=' is in build-ffx-own.ps1 %d time(s), not once" % (v, n))
    open(script, "wb").write(d)
    refused("a copy of the script that does not clear the environment, built with _CL_=/fp:fast")
    open(script, "wb").write(keep)
    swap(script, b"'/c', '/O2', '/Ob2'", b"'/c', '/O1', '/Ob2'")
    refused("a copy of the script with /O2 changed to /O1")
    open(script, "wb").write(keep)
    glue = os.path.join(third, "ffx-own", "glue", "own_no_swapchain.cpp")
    keep_glue = swap(glue, b"return FFX_ERROR_INVALID_ARGUMENT;", b"return FFX_OK;")
    refused("a copy with one line of our glue changed")
    # a failed build leaves no library: a library stands in that output folder now (the control's own, just built)
    out3 = os.path.join(third, "obj", "ffx-own")
    need(os.path.isfile(os.path.join(out3, "ffx-own-fg.dll")), "(a library stands in the third copy's output folder)")
    open(glue, "wb").write(keep_glue)
    swap(glue, b"return FFX_ERROR_INVALID_ARGUMENT;", b"int planted_by_the_proof; return FFX_ERROR_INVALID_ARGUMENT;")
    dll, said, rc = build(third)
    control(rc != 0 and dll is None and "warning C4101" in said and "No library was delivered" in said and os.path.isfile(os.path.join(out3, "build.log")),
            "a compile warning planted in that copy: the build fails and NO ffx-own-fg.dll is left where one stood", "the folder holds: %s" % ", ".join(sorted(os.listdir(out3))))
    # a folder with a file named like AMD's is not a build folder of ours
    open(glue, "wb").write(keep_glue)
    open(os.path.join(out3, "amd_fidelityfx_a_marker_of_the_proof.dll"), "wb").write(b"not AMD's and not a library: a marker of the proof\r\n")
    before = listing(out3)
    dll, said, rc = build(third)
    control(rc != 0 and "does not build into such a folder" in said and listing(out3) == before,
            "a file named like AMD's in the output folder: the script stops before it deletes or writes anything there", "%d files, each as it was" % len(before))
    os.remove(os.path.join(out3, "amd_fidelityfx_a_marker_of_the_proof.dll"))
    p = pe_read.PE(here)
    need(p.machine == 0x8664 and p.is_dll(), "it is a 64-bit DLL")
    own, names = p.exports()
    need(own == "ffx-own-fg.dll" and names == FIVE, "it calls itself ffx-own-fg.dll and exports AMD's five entry points and nothing else", "%r %s" % (own, names))
    imp = p.imports()
    need(not [d for d in imp if RUNTIME.match(d)], "it imports no VC++ runtime DLL: %s" % ", ".join("%s (%d)" % (d, len(imp[d])) for d in sorted(imp)))
    need(sorted(d.lower() for d in imp) == ["dxgi.dll", "kernel32.dll"] and imp[[d for d in imp if d.lower() == "dxgi.dll"][0]] == ["CreateDXGIFactory2"],
         "its imports are KERNEL32.dll and CreateDXGIFactory2 of dxgi.dll: no d3d12.dll, no WINMM, no USER32, nothing of AMD's")
    need(not p.has_resources() and not p.signed(), "it carries no resource (so no version resource, AMD's or anyone's) and no signature")
    need(not [e for e in p.debug_entries() if e[0] == 2], "it holds no PDB path (no CodeView debug entry)")
    low = p.d.lower()
    need(b"amd_fidelityfx_framegeneration" not in low and b"amd_fidelityfx_upscaler" not in low and b"amd_fidelityfx_loader" not in low,
         "none of AMD's three library names is in it: it loads none of them")
    named = [os.path.join(dp, f) for r in (os.path.join(ROOT, "obj"), tmp) for dp, dn, fn in os.walk(r) for f in fn if f.lower().startswith("amd_fidelityfx")]
    need(not named, "no file in any build folder of this run is named like a file of AMD's", "; ".join(named[:3]))
    return here, size, h, tool, third


# ---- 5 ------------------------------------------------------------------------------------------------------------------
def reads_of(path):
    """a build's reads.txt -> {path, lower case: git blob id of what was read}"""
    out = {}
    for l in open(path).read().splitlines():
        f = l.split()
        if len(f) != 2 or not re.fullmatch(r"[0-9a-f]{40}", f[0]): raise Failed("%s: a line that is not 'git blob id, path': %s" % (path, l))
        out[f[1].lower()] = f[0]
    return out


def part_reads(tmp, rows, facts, third):
    print("5. the build reads what the manifest says it reads, by its bytes")
    cpp = reads_of(os.path.join(ROOT, "obj", "ffx-own", "reads.txt"))
    hlsl = {p.lower(): i for p, i in facts["read_ids"].items()}
    own = {here[len(M.FOLDER) + 1:].lower(): bid for use, bid, _, _, here in rows if use == "own"}
    read = dict(hlsl); read.update(cpp)
    need(set(read) == set(own), "cl opened %d of AMD's files and dxc %d (%d of them both): together exactly the manifest's %d rows marked \"own\"" % (len(cpp), len(hlsl), len(set(cpp) & set(hlsl)), len(own)),
         "read and not listed: %s; listed and not read: %s" % (sorted(set(read) - set(own))[:4], sorted(set(own) - set(read))[:4]))
    wrong = sorted(p for p in own if cpp.get(p, own[p]) != own[p] or hlsl.get(p, own[p]) != own[p])
    need(not wrong, "and by their bytes: each was hashed in the copy it was compiled from and has the manifest's git blob id, all %d" % len(own), "not the manifest's bytes: %s" % ", ".join(wrong[:4]))
    victim = "framegeneration/fsr3/internal/ffx_frameinterpolation.cpp"
    swap(os.path.join(third, "external", "ffx", "ffx_api_2.3.0", victim.replace("/", os.sep)), b"cosf(0.5f * params->cameraFovAngleVertical)", b"cosf(0.6f * params->cameraFovAngleVertical)")
    built(third)
    theirs = reads_of(os.path.join(third, "obj", "ffx-own", "reads.txt"))
    control(sorted(p for p in theirs if theirs[p] != own.get(p)) == [victim], "a copy of the inputs with one character changed in AMD's ffx_frameinterpolation.cpp builds, and its list is refused for exactly that file",
            "it read %s..., the manifest says %s..." % (theirs[victim][:12], own[victim][:12]))
    foreign = os.path.join(tmp, "a foreign include folder"); os.makedirs(foreign)
    shutil.move(os.path.join(third, "ffx-own", "glue", "pix3.h"), os.path.join(foreign, "pix3.h"))
    dll, said, rc = build(third, dict(ENV, INCLUDE=foreign))
    control(rc != 0 and dll is None and "the compiler opened" in said and "pix3.h, which is neither in this build's copy of the source nor a file of the compiler or of the Windows SDK" in said,
            "a header taken from a folder that only the INCLUDE variable names: the build fails, names the file, and delivers no library")
    swap_ = [h for _, _, _, _, h in rows if "swapchain" in h.lower()]
    need(not swap_, "no file of AMD's frame interpolation swapchain is in the repository's kit", "; ".join(swap_[:3]))


# ---- 6 ------------------------------------------------------------------------------------------------------------------
def part_probe(tmp, dll):
    print("6. the library answers as AMD's does (no device)")
    vt = open(os.path.join(M.FFX, "VERSION.txt"), encoding="utf-8", errors="replace").read()
    m = re.search(r'one frame generation provider, "([\d.]+)" \(id\s+0x([0-9A-Fa-f]+)\)', vt)
    need(m is not None, "external\\ffx\\VERSION.txt records what AMD's own library answered the helper")
    name, pid = m.group(1), m.group(2).upper()
    need((name, pid) == ("3.1.6", "F600000000C01006"), "and it is \"3.1.6\", id 0xF600000000C01006")
    out = os.path.join(tmp, "probe")
    rc, said = run(PS + [os.path.join(ROOT, "tools", "ffx-own", "build-probe.ps1"), "-OutDir", out])
    exe = os.path.join(out, "ffx-own-probe.exe"); ctl = os.path.join(out, "ffx-own-probe-control.dll")
    need(rc == 0 and os.path.isfile(exe) and os.path.isfile(ctl), "the probe and its control library are built", said.strip()[-500:])
    rc, said = run([exe, dll, name, pid])
    need(rc == 0 and "entry points: create 1 destroy 1 configure 1 query 1 dispatch 1" in said, "ffx-own-fg.dll loads by its full path, and AMD's ffxLoadFunctions finds all five entry points in it", said.strip()[-400:])
    need(('frame generation: query returned 0, 1 provider(s)\n  "%s" id 0x%s\n' % (name, pid)) in said and said.rstrip().endswith('PASS: one frame generation provider, "%s" id 0x%s' % (name, pid)),
         "asked with a null device, it lists one frame generation provider: \"%s\" id 0x%s" % (name, pid), said.strip()[-400:])
    need("frame generation swapchain: query returned 0, 0 provider(s)" in said and "upscale: query returned 0, 0 provider(s)" in said,
         "and no swapchain provider and no upscaler: it stands in nobody's place on the upscaler's side")
    rc, said = run([exe, ctl, name, pid])
    control(rc == 1 and 'the first "3.1.5" id 0xF600000000C01005' in said and "FAIL: " in said, "the probe fails on a library that answers \"3.1.5\"", "exit code 1")
    p = pe_read.PE(ctl)
    need(p.exports()[1] == FIVE, "(that control library exports the same five names: it failed for its answer, not for a missing entry point)")


# ---- 7 ------------------------------------------------------------------------------------------------------------------
def part_alt(tmp, ref_sha, ref2_sha):
    print("7. the two test libraries can be made (the controls of the next stage's picture compare), each behind both interfaces")
    out = os.path.join(tmp, "alt"); out2 = os.path.join(tmp, "alt2")
    for which in ("fi", "of"):
        try: f = build_alt.build(which, out, ENV, out2)
        except build_alt.Stopped as e: raise Failed(str(e))
        note = open(f["note"], encoding="ascii").read()
        need(f["sha256"] != ref_sha and os.path.basename(f["path"]) == f["name"] + ".dll" and ("line   %d" % f["line"]) in note and f["new"] in note,
             "%s.dll: %d bytes, SHA-256 %s..., from a temp copy of the kit with line %d of %s changed (\"%s\" is \"%s\"); the two headers of that pass differ and no other"
             % (f["name"], f["size"], f["sha256"][:16], f["line"], f["file"].split("/")[-1], f["old"].split("=", 1)[1].strip(), f["new"].split("=", 1)[1].strip()))
        g = f.get("second")
        need(g is not None and g["sha256"] not in (ref_sha, f["sha256"]) + tuple(ref2_sha) and os.path.basename(g["path"]) == g["name"] + ".dll" and os.path.isfile(g["note"]),
             "%s.dll: %d bytes, SHA-256 %s..., THE SECOND LIBRARY from the same altered tree: not a recorded library, it calls itself by its test name, exports the interface's seven names, and its objects link to the altered library above"
             % ((g or {}).get("name"), (g or {}).get("size", 0), (g or {}).get("sha256", "?")[:16]))
    need(sorted(f for f in os.listdir(out) if f.endswith(".dll")) == ["ffx-own-fg-alt-fi.dll", "ffx-own-fg-alt-of.dll"], "both stand in one folder: the second build removed nothing of the first's")
    need(sorted(f for f in os.listdir(out2) if f.endswith(".dll")) == ["dlssiv-fg-alt-fi.dll", "dlssiv-fg-alt-of.dll"], "and so do the second library's two, in a folder of their own")
    before = listing(os.path.join(ROOT, "obj", "ffx-own"))
    rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-TestName", "alt-fi", "-OutDir", os.path.join(tmp, "alt-refused")])
    said = re.sub(r"\s+", " ", said)
    control(rc != 0 and "this tree is not one (no ffx-own\\ALTERED.txt)" in said and not os.path.isdir(os.path.join(tmp, "alt-refused")) and listing(os.path.join(ROOT, "obj", "ffx-own")) == before,
            "this tree, which is not altered, refuses -TestName and builds nothing: the unaltered source cannot come out under a test's name")


# ---- 8 ------------------------------------------------------------------------------------------------------------------
def product_of(root):
    return os.path.join(root, "obj", "dlssiv-fg", "dlssiv-fg.dll")


def exact_of(root):
    return os.path.join(root, "obj", "dlssiv-fg-exact", "dlssiv-fg-exact.dll")


def build2(root, env=None, extra=()):
    """the second library's build: THE REFERENCE (no cut) and, beside it, the product. -> (the reference or None, what the
    script said, its exit code). The product of the same run is product_of(root)."""
    rc, out = run(PS + [os.path.join(root, "build-ffx-own.ps1"), "-Target", "dlssiv-fg"] + list(extra), env=env)
    out = re.sub(r"\s+", " ", out)
    dll = os.path.join(root, "obj", "dlssiv-fg-ref", "dlssiv-fg-ref.dll")
    return (dll if os.path.isfile(dll) else None), out, rc


def built2(root, env=None):
    dll, out, rc = build2(root, env)
    if rc != 0 or dll is None or not os.path.isfile(product_of(root)) or not os.path.isfile(exact_of(root)) or re.search(r"\bwarning\s+(C|LNK|D)\d+", out): raise Failed("build-ffx-own.ps1 -Target dlssiv-fg in %s: exit code %d: %s" % (root, rc, out.strip()[-700:]))
    return dll, out


def objects_of(path):
    """a second library's objects.txt -> ((the sibling's SHA-256, its size, its name), {object: SHA-256})"""
    sib, objs = None, {}
    for l in open(path, encoding="ascii").read().splitlines():
        f = l.split()
        if len(f) == 4 and f[0] == "sibling" and sib is None: sib = (f[1], int(f[2]), f[3])
        elif len(f) == 3 and f[0] == "object": objs[f[2]] = f[1]
        else: raise Failed("%s: a line that is neither the sibling's nor an object's: %s" % (path, l))
    if sib is None: raise Failed("%s names no sibling" % path)
    return sib, objs


def probe_fg(dll):
    """tools\\ffx-own\\probe_fg.py on a library, in a process of its own (the library is mapped there and nowhere else); -> (exit code, what it said)"""
    return run([sys.executable, os.path.join(ROOT, "tools", "ffx-own", "probe_fg.py"), dll])


def part_second(tmp, rec, rec2, first):
    print("8. the second library with no cut, THE REFERENCE (dlssiv-fg-ref.dll: stage 2's dlssiv-fg.dll): the mod's own interface over the same objects of AMD's")
    t0 = time.time()
    here, said = built2(ROOT)
    folder = os.path.dirname(here)
    size = os.path.getsize(here); h = sha(here)
    tool = R.read_toolchain(os.path.join(folder, "toolchain.txt"))
    need(all(tool.get(k) for k in R.TOOLCHAIN), "built in this tree with cl %s, link %s, Windows SDK %s: dlssiv-fg.dll, %d bytes, SHA-256 %s" % (tool.get("cl"), tool.get("link"), tool.get("windows-sdk"), size, h))
    why = R.toolchain_refusal(tool, rec)
    need(not why, "the toolchain is the first record's", why)
    sib, objs = objects_of(os.path.join(folder, "objects.txt"))
    need(sib == (rec["sha256"], int(rec["size"]), "ffx-own-fg.dll"), "THE OBJECTS OF AMD'S FILES IN IT ARE THE REFERENCE LIBRARY'S: the objects of this one compiler run link to ffx-own-fg.dll with the FIRST record's size and SHA-256 (%s...)" % rec["sha256"][:16],
         "its sibling is %s, %d bytes, SHA-256 %s" % (sib[2], sib[1], sib[0]))
    need(sorted(objs) == sorted(ELEVEN), "it is linked from eleven objects: nine of AMD's twelve files, the swapchain's two stubs and the interface", ", ".join(sorted(objs)))
    mp = open(os.path.join(folder, "dlssiv-fg-ref.map"), encoding="latin1").read()
    need(all(o in mp for o in ELEVEN) and not [o for o in NOT_LINKED if o in mp], "its map names each of the eleven and no object of AMD's ffx-api core, of its query fallback, of its provider layer or of our provider list",
         "named and not allowed: %s" % ", ".join(o for o in NOT_LINKED if o in mp))
    need(reads_of(os.path.join(folder, "reads.txt")) == reads_of(os.path.join(os.path.dirname(first), "reads.txt")), "the compiler opened the same files of AMD's as for the first library, each with the git blob id part 5 held to the manifest")
    ref = dict(rec2) if rec2 else {}
    if NEW_FG:
        ref.update({"library": "dlssiv-fg.dll", "size": str(size), "sha256": h}); ok("NEW RECORD (the second library's): that library")
    else:
        need(rec2 is not None and all(rec2[k] == rec[k] for k in R.KEYS if k not in ("library", "size", "sha256")), "the two records name one toolchain, one dxc, one set of headers and one manifest",
             "they differ in: %s" % ", ".join(k for k in R.KEYS if rec2 and k not in ("library", "size", "sha256") and rec2[k] != rec[k]))
        why = R.library_refusal(here, rec2, os.path.basename(R.PATH_FG))
        need(not why, "the library is the second record's: %s bytes, SHA-256 %s" % (rec2["size"], rec2["sha256"]), why)
    need(h != rec["sha256"], "and it is not the first record's library")
    fourth = os.path.join(tmp, "a fourth folder, for the second library", "DLSS-IV")
    stage(fourth)
    there, said = built2(fourth, dict(ENV, _CL_="/fp:fast"))
    out4 = os.path.dirname(there)
    need(os.path.normcase(folder) != os.path.normcase(out4) and os.path.getmtime(there) >= t0 and os.path.getmtime(here) >= t0 and "_CL_ was set in the environment ('/fp:fast'): cleared" in said,
         "built again from a copy of its inputs in %s, with _CL_=/fp:fast in the environment, which the script said it cleared" % fourth)
    need(open(here, "rb").read() == open(there, "rb").read(), "the two libraries are byte-equal", "this tree %s, the copy %s" % (h, sha(there)))
    need(sorted(os.listdir(out4)) == SIX_REF, "and the build left its six files there and nothing else: no object, no response file, and NOT the stage 1 library it linked on the way",
         ", ".join(sorted(os.listdir(out4))))
    prod4 = product_of(fourth)
    need(sorted(os.listdir(os.path.dirname(prod4))) == SEVEN_FILES, "and beside that folder the product's, with its seven files and nothing else", ", ".join(sorted(os.listdir(os.path.dirname(prod4)))))
    prod4 = (sha(prod4), os.path.getsize(prod4))                # the product from this second folder, before the controls below change the copy: part 10 holds it to this tree's
    exact4 = exact_of(fourth)
    need(sorted(os.listdir(os.path.dirname(exact4))) == SEVEN_EXACT, "and the exact library's, with its seven files and nothing else", ", ".join(sorted(os.listdir(os.path.dirname(exact4)))))
    exact4 = (sha(exact4), os.path.getsize(exact4))             # and the exact library from there: part 9 holds it to this tree's
    # the controls, each from that copy with one thing changed
    glue = os.path.join(fourth, "ffx-own", "glue", "dlssiv_fg.cpp")
    keep = swap(glue, b"f.opticalFlowBlockSize = 8;", b"f.opticalFlowBlockSize = 4;")
    dll, _ = built2(fourth)
    why = R.library_refusal(dll, ref, os.path.basename(R.PATH_FG))
    sib4 = objects_of(os.path.join(out4, "objects.txt"))[0]
    control(bool(why) and sha(dll) in why and ref["sha256"] in why and sib4[0] == rec["sha256"], "one line of our interface changed in a copy (the block size it hands AMD's interpolation): a library the second record refuses, with both hashes; and its sibling is still the first record's, AMD's objects did not change",
            "%d bytes, SHA-256 %s..." % (os.path.getsize(dll), sha(dll)[:16]))
    open(glue, "wb").write(keep)
    swap(os.path.join(fourth, "external", "ffx", "ffx_api_2.3.0", "framegeneration", "fsr3", "internal", "ffx_frameinterpolation.cpp"), b"cosf(0.5f * params->cameraFovAngleVertical)", b"cosf(0.6f * params->cameraFovAngleVertical)")
    dll, _ = built2(fourth)
    sib4 = objects_of(os.path.join(out4, "objects.txt"))[0]
    control(sib4[0] != rec["sha256"] and sha(dll) != ref["sha256"], "one character changed in AMD's ffx_frameinterpolation.cpp in that copy: its sibling is NOT the first record's library, and the library is not the second record's",
            "the sibling is %s..." % sib4[0][:16])
    before = listing(out4)
    dll, said, rc = build2(fourth, extra=["-DynamicCrt"])
    control(rc != 0 and "it is not built with -Target dlssiv-fg" in said and listing(out4) == before, "-DynamicCrt with this target is refused, and the output folder is as it was")
    p = pe_read.PE(here)
    need(p.machine == 0x8664 and p.is_dll(), "it is a 64-bit DLL")
    own, names = p.exports()
    need(own == "dlssiv-fg.dll" and names == SEVEN and not [n for n in names if n in FIVE], "its bytes call themselves dlssiv-fg.dll, as stage 2's did (the reference's name is its FILE's), and it exports the interface's seven functions and nothing else: none of AMD's five entry points", "%r %s" % (own, names))
    imp = p.imports()
    need(not [d for d in imp if RUNTIME.match(d)], "it imports no VC++ runtime DLL: %s" % ", ".join("%s (%d)" % (d, len(imp[d])) for d in sorted(imp)))
    need(sorted(d.lower() for d in imp) == ["dxgi.dll", "kernel32.dll"] and imp[[d for d in imp if d.lower() == "dxgi.dll"][0]] == ["CreateDXGIFactory2"],
         "its imports are KERNEL32.dll and CreateDXGIFactory2 of dxgi.dll: no d3d12.dll, no WINMM, no USER32, nothing of AMD's")
    need(not p.has_resources() and not p.signed(), "it carries no resource (so no version resource: the package stage gives it the mod's own, never AMD's) and no signature")
    need(not [e for e in p.debug_entries() if e[0] == 2], "it holds no PDB path (no CodeView debug entry)")
    need(b"amd_fidelityfx" not in p.d.lower(), "the text \"amd_fidelityfx\" is nowhere in it: it names no library of AMD's and loads none")
    rc, said = probe_fg(here)
    need(rc == 0 and 'interface 1, stage 2, AMD "3.1.6" id 0xF600000000C01006' in said and said.rstrip().endswith("PASS"), "loaded by its full path in a process of its own, with no device, it says what it is: interface 1, stage 2, AMD's \"3.1.6\", id 0xF600000000C01006; and it refuses a description of another size, a create with nothing and a destroy of nothing with the interface's own codes",
         said.strip()[-400:])
    rc, said = probe_fg(first)
    control(rc == 1 and "lacks dlssivFgInfo" in said, "the same probe on stage 1's library fails: it has none of the seven functions", "exit code 1")
    named = [os.path.join(dp, f) for r in (os.path.join(ROOT, "obj"), tmp) for dp, dn, fn in os.walk(r) for f in fn if f.lower().startswith("amd_fidelityfx")]
    need(not named, "no file in any build folder of this run is named like a file of AMD's", "; ".join(named[:3]))
    return here, size, h, fourth, prod4, exact4


# ---- 9 ------------------------------------------------------------------------------------------------------------------
def cut_facts(path):
    """a library's cuts.txt -> {"cuts": n, "cuts-sha256": .., "product": .., "exact": .., "names": [...], "edits": [(cut, before, after, file)], "headers": {file: sha256}}"""
    out = {"names": [], "edits": [], "headers": {}}
    for l in open(path, encoding="ascii").read().splitlines():
        f = l.split()
        if not f or l.startswith("  "): continue
        if f[0] in ("cuts", "cuts-sha256", "product", "exact"): out[f[0]] = f[1]
        elif f[0] == "cut": out["names"].append(f[1])
        elif f[0] == "edit": out["edits"].append((f[1], f[2], f[3], f[4]))
        elif f[0] == "header": out["headers"][f[2]] = f[1]
    return out


def states():
    """{cut: state} of ffx-own\\cuts\\CUTS.txt, read here by itself (not through tools\\ffx-own\\cuts.py, which this part holds)"""
    listed = [l.split() for l in open(os.path.join(ROOT, "ffx-own", "cuts", "CUTS.txt"), encoding="ascii").read().splitlines() if l.strip() and not l.startswith("#")]
    return [(f[0], f[1]) for f in listed if len(f) == 2]


def part_exact(tmp, rec, rec2, recx, ref, fourth, exact4):
    print("9. THE EXACT LIBRARY: dlssiv-fg-exact.dll, the cuts marked \"in\" and no picture cut, built by the same run between the reference and the product")
    here = exact_of(ROOT)
    folder = os.path.dirname(here)
    size = os.path.getsize(here); h = sha(here)
    names = C.chosen("exact")
    cdigest = C.digest(names)
    table = states()
    need(names == [n for n, s in table if s == "in"], "its cuts are the ones ffx-own\\cuts\\CUTS.txt marks \"in\": %d" % len(names))
    need(sorted(os.listdir(folder)) == SEVEN_EXACT, "its folder holds its seven files and nothing else", ", ".join(sorted(os.listdir(folder))))
    lines = [l.split() for l in open(os.path.join(folder, "objects.txt"), encoding="ascii").read().splitlines()]
    sib = [l for l in lines if l[0] == "sibling"]; refl = [l for l in lines if l[0] == "reference"]; objs = sorted(l[2] for l in lines if l[0] == "object")
    need(len(sib) == 1 and sib[0][1] == rec["sha256"] and len(refl) == 1 and refl[0][1] == rec2["sha256"] and refl[0][1] == sha(ref) and objs == sorted(ELEVEN),
         "ITS OWN LIST NAMES WHAT IT WAS BUILT BESIDE: the first record's library as the sibling and stage 2's record's as the reference (the file part 8 held), and its eleven objects: it is that source, that compiler and those options, plus its cuts")
    facts = cut_facts(os.path.join(folder, "cuts.txt"))
    need(facts.get("cuts") == str(len(names)) and facts.get("cuts-sha256") == cdigest and facts.get("exact") == "yes" and facts.get("product") == ("yes" if names == C.chosen("") else "no") and facts["names"] == names,
         "cuts.txt beside it says the cuts that are in: %d (%s), digest %s..., the set the list marks \"in\", and that it is the exact library" % (len(names), ", ".join(names) or "none", cdigest[:16]), str(facts)[:300])
    try: files, gnames, files2, gnames2 = C.generate_committed(picture=True)
    except C.Refused as e: raise Failed("the headers of the cuts: %s" % e)
    bad = C.compare_committed(files)
    need(not bad and gnames == names, "THE HEADERS OF THE EXACT CUTS ARE THE CUT SOURCE'S: all 18 passes compiled again from a copy the cuts were applied to; the %d header(s) that differ from ffx-own\\shaders are the committed ones of ffx-own\\shaders-cut, byte for byte, and that folder holds nothing else" % len(files), "; ".join(bad[:3]))
    need({f: hashlib.sha256(t.encode("ascii")).hexdigest() for f, t in files.items()} == facts["headers"], "and those are the headers the build put into its copy (cuts.txt names each with its SHA-256)")
    hcount, hdigest = (R.headers_digest(C.committed()) if os.path.isdir(C.committed()) else (0, hashlib.sha256(b"").hexdigest()))
    bad, n, total = M.check()
    need(not bad, "AMD'S VENDORED FILES ARE STILL AMD'S after every build of this run: %d files, each the manifest's (the cuts' edits were made in the builds' temp copies and nowhere else)" % n, "; ".join(bad[:3]))
    refx = dict(recx) if recx else {}
    if NEW_EXACT:
        refx = dict(rec, library="dlssiv-fg-exact.dll", size=str(size), sha256=h); refx.update({"cuts": str(len(names)), "cuts-sha256": cdigest, "cut-headers": str(hcount), "cut-headers-sha256": hdigest})
        ok("NEW RECORD (the exact library's): that library, %d bytes, SHA-256 %s, with %d cut(s)" % (size, h, len(names)))
    else:
        need(recx is not None and all(recx[k] == rec[k] for k in R.KEYS if k not in ("library", "size", "sha256")), "the exact library's record names the toolchain, the dxc, the headers and the manifest of the other records",
             "they differ in: %s" % ", ".join(k for k in R.KEYS if recx and k not in ("library", "size", "sha256") and recx[k] != rec[k]))
        need(recx["library"] == "dlssiv-fg-exact.dll", "its record's \"library\" is the exact library's own file name, dlssiv-fg-exact.dll: a reader that wants a record of dlssiv-fg.dll (the kit's installer) cannot be handed this one", recx["library"])
        need((recx["cuts"], recx["cuts-sha256"], recx["cut-headers"], recx["cut-headers-sha256"]) == (str(len(names)), cdigest, str(hcount), hdigest),
             "the exact cuts and their headers are its record's: %s cut(s), digest %s...; %s header(s), digest %s..." % (recx["cuts"], recx["cuts-sha256"][:16], recx["cut-headers"], recx["cut-headers-sha256"][:16]),
             "this tree: %d cut(s) %s, %d header(s) %s" % (len(names), cdigest, hcount, hdigest))
        why = R.library_refusal(here, recx, os.path.basename(R.PATH_EXACT))
        need(not why, "THE LIBRARY IS THE EXACT LIBRARY'S RECORD'S: %s bytes, SHA-256 %s" % (recx["size"], recx["sha256"]), why)
    need(exact4 == (h, size), "the same bytes from the second folder (part 8's copy of the inputs, with _CL_=/fp:fast in the environment)", "this tree %s, the copy %s" % (h, exact4[0]))
    same = open(here, "rb").read() == open(ref, "rb").read()
    need(same == (not names), "with no exact cut the exact library IS the reference, byte for byte" if not names else "with %d cut(s) in it is not the reference's bytes" % len(names))
    p = pe_read.PE(here)
    own, exported = p.exports()
    imp = p.imports()
    need(p.machine == 0x8664 and p.is_dll() and own == "dlssiv-fg.dll" and exported == SEVEN and not [d for d in imp if RUNTIME.match(d)] and sorted(d.lower() for d in imp) == ["dxgi.dll", "kernel32.dll"]
         and not p.has_resources() and not p.signed() and not [e for e in p.debug_entries() if e[0] == 2] and b"amd_fidelityfx" not in p.d.lower(),
         "it is a 64-bit DLL whose bytes call themselves dlssiv-fg.dll, as the product did while it was this library (the exact library's name is its FILE's), exports the interface's seven functions and nothing else, imports KERNEL32.dll and dxgi.dll alone, carries no resource, no signature and no PDB path, and names no library of AMD's")
    rc, said = probe_fg(here)
    # (2026-10-17) a library with the cut fi-position says interface 2 (ffx-own\glue\dlssiv_fg.h); one without it, 1
    # (2026-10-18) and one with fi-store-apart as well, 3
    want = 'interface %d, stage %d, AMD "3.1.6" id 0xF600000000C01006' % (3 if "fi-store-apart" in names else 2 if "fi-position" in names else 1, 3 if names else 2)
    need(rc == 0 and want in said and said.rstrip().endswith("PASS") and (not names or ("%d cut(s) %s" % (len(names), cdigest[:8])) in said),
         "with no device it says what it is: %s%s" % (want, (", and \"%d cut(s) %s\": the count and the digest's first eight digits" % (len(names), cdigest[:8])) if names else ""), said.strip()[-400:])
    # the controls
    x0 = os.path.join(tmp, "cuts-none")
    rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "none", "-OutDir", x0])
    d0 = os.path.join(x0, "dlssiv-fg.dll")
    control(rc == 0 and os.path.isfile(d0) and sha(d0) == rec2["sha256"] and not os.path.isdir(x0 + "-ref") and not os.path.isdir(x0 + "-exact"), "-Cuts none builds the reference's bytes (the record of stage 2), into the folder it was given, and delivers no second copy of the reference and no exact library: every cut has a switch", re.sub(r"\s+", " ", said)[-300:] if rc else "")
    if names:
        x1 = os.path.join(tmp, "cuts-less")
        rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "exact,-" + names[0], "-OutDir", x1])
        d1 = os.path.join(x1, "dlssiv-fg.dll")
        control(rc == 0 and os.path.isfile(d1) and sha(d1) != h,
                "the exact library less its first cut (-Cuts exact,-%s) is another library: that cut is in the exact library's bytes" % names[0], "SHA-256 %s..." % (sha(d1)[:16] if os.path.isfile(d1) else "?"))
    pfolder = os.path.dirname(product_of(ROOT))
    before = listing(pfolder); beforex = listing(folder)
    rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "none"])
    said = re.sub(r"\s+", " ", said)
    control(rc != 0 and "give it -OutDir, a folder of its own" in said and listing(pfolder) == before and listing(folder) == beforex, "-Cuts without a folder of its own is refused, and the product's folder and the exact library's are as they were")
    # (2026-10-11) A CUT THAT CHANGES THE PICTURE (CUTS.txt: "picture") IS BUILT ONLY INTO A VARIANT LIBRARY OF ITS OWN NAME,
    # and a variant's name is given to no library without one: the two refusals that keep such a cut out of the product.
    # (2026-10-12) AND NO CUT THAT CHANGES THE PICTURE, ACCEPTED OR NOT, IS EVER AMONG THE EXACT LIBRARY'S
    pics = [n for n, s in table if s == "picture"]; acc = [n for n, s in table if s == "accepted"]
    need(not [n for n in names if n in pics or n in acc], "no cut that changes the picture is among the exact library's cuts (CUTS.txt lists %d such that nobody accepted, each built only into a variant library, and %d accepted, which are the product's alone)" % (len(pics), len(acc)))
    if pics:
        xp = os.path.join(tmp, "cuts-picture")
        rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "exact,+" + pics[0], "-OutDir", xp])
        said = re.sub(r"\s+", " ", said)
        control(rc != 0 and "CHANGES THE PICTURE" in said and not [f for f in os.listdir(xp) if f.lower().endswith(".dll")],
                "a cut that changes the picture and that nobody accepted (-Cuts exact,+%s) is refused in a build that is not a variant, and no library is delivered" % pics[0])
        xv = os.path.join(tmp, "cuts-variant")
        rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Variant", "plain", "-Cuts", "exact", "-OutDir", xv])
        said = re.sub(r"\s+", " ", said)
        control(rc != 0 and "no cut of its set changes the picture" in said and not [f for f in os.listdir(xv) if f.lower().endswith(".dll")],
                "a variant's name is refused to a set with no cut that changes the picture (-Variant plain -Cuts exact), and no library is delivered")
    cutfile = os.path.join(fourth, "ffx-own", "cuts", "control-one-pixel.cut")
    d = open(cutfile, "rb").read()
    i = d.find(b"    if (bWriteColor)")                    # the first one: the cut's OLD text
    if i < 0: raise Failed("the one-pixel control's cut has no line 'if (bWriteColor)' to change")
    open(cutfile, "wb").write(d[:i] + b"    if (bWriteColour)" + d[i + len(b"    if (bWriteColor)"):])
    x2 = os.path.join(tmp, "cuts-stale")
    rc, said = run(PS + [os.path.join(fourth, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-TestName", "px", "-OutDir", x2])
    said = re.sub(r"\s+", " ", said)
    control(rc != 0 and "cut control-one-pixel: its old text is in framegeneration/fsr3/include/gpu/frameinterpolation/ffx_frameinterpolation_inpainting.h 0 time(s), not once" in said
            and not [f for f in os.listdir(x2) if f.endswith(".dll")], "a cut whose old text is not in AMD's file (one word changed in a copy of the one-pixel control) stops the build with a line that names the cut and the file, and no library is delivered", said[-300:])
    open(cutfile, "wb").write(d)
    x3 = os.path.join(tmp, "px")
    rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-TestName", "px", "-OutDir", x3])
    dpx = os.path.join(x3, "dlssiv-fg-px.dll")
    if rc != 0 or not os.path.isfile(dpx): raise Failed("build-ffx-own.ps1 -Target dlssiv-fg -TestName px: exit code %d: %s" % (rc, re.sub(r"\s+", " ", said)[-500:]))
    fpx = cut_facts(os.path.join(x3, "cuts.txt")); ppx = pe_read.PE(dpx)
    need(sha(dpx) not in (h, rec["sha256"], rec2["sha256"]) and fpx["names"] == names + ["control-one-pixel"] and fpx.get("product") == "no" and fpx.get("exact") == "no" and ppx.exports() == ("dlssiv-fg-px.dll", SEVEN) and os.path.isfile(os.path.join(x3, "dlssiv-fg-px.txt")),
         "THE JUDGE'S ONE-PIXEL CONTROL is made (dlssiv-fg-px.dll, %d bytes, SHA-256 %s...): not a recorded library; its cuts are THE EXACT LIBRARY'S and the control on top, with no picture cut of the product's (it must differ from AMD's picture by its one pixel alone); it calls itself by its test name and exports the same seven names" % (os.path.getsize(dpx), sha(dpx)[:16]))
    return here, size, h, refx, files2, gnames2, sha(dpx)


# ---- 10 -----------------------------------------------------------------------------------------------------------------
def read_pin():
    """tools\\ffx-own\\PICTURES-dlssiv-fg.txt -> {key: [values]} (its "key value ..." lines), or None when it is not there.
    tools\\test-fg-own\\run.py writes it (--ours --new-pictures) and holds the product's pictures to it on the card; this proof
    has no device and holds only which picture cuts it was made for."""
    if not os.path.isfile(R.PATH_PICTURES): return None
    out = {}
    for l in open(R.PATH_PICTURES, encoding="ascii").read().splitlines():
        f = l.split()
        if f and not l.startswith("#"): out[f[0]] = f[1:]
    return out


def part_product(tmp, rec, rec2, refx, rec3, ref, exact, fourth, prod4, files2, gnames2, pxsha):
    print("10. THE PRODUCT: dlssiv-fg.dll, the exact library's cuts and the accepted picture cuts on top of them, built by the same run from the copy the exact library was compiled from")
    here = product_of(ROOT)
    folder = os.path.dirname(here)
    size = os.path.getsize(here); h = sha(here)
    hx = sha(exact)
    table = states()
    xnames = C.chosen("exact"); names = C.chosen(""); acc = names[len(xnames):]
    cdigest = C.digest(names)
    need(names[:len(xnames)] == xnames and acc == [n for n, s in table if s == "accepted"], "its cuts are the exact library's %d, in their order, and then the %d the list marks \"accepted\": %s" % (len(xnames), len(acc), ", ".join(acc) or "none"))
    need(sorted(os.listdir(folder)) == SEVEN_FILES, "its folder holds its seven files and nothing else", ", ".join(sorted(os.listdir(folder))))
    lines = [l.split() for l in open(os.path.join(folder, "objects.txt"), encoding="ascii").read().splitlines()]
    sib = [l for l in lines if l[0] == "sibling"]; refl = [l for l in lines if l[0] == "reference"]; exl = [l for l in lines if l[0] == "exact"]; objs = sorted(l[2] for l in lines if l[0] == "object")
    need(len(sib) == 1 and sib[0][1] == rec["sha256"] and len(refl) == 1 and refl[0][1] == rec2["sha256"] and refl[0][1] == sha(ref) and len(exl) == 1 and exl[0][1] == refx["sha256"] == hx and exl[0][3] == "dlssiv-fg-exact.dll" and objs == sorted(ELEVEN),
         "ITS OWN LIST NAMES WHAT IT WAS BUILT BESIDE AND ON: the first record's library as the sibling, stage 2's record's as the reference, THE EXACT LIBRARY'S RECORD'S as the exact library (the file part 9 held), and its eleven objects: it is the exact library's source plus its picture cuts, and nothing else")
    facts = cut_facts(os.path.join(folder, "cuts.txt"))
    need(facts.get("cuts") == str(len(names)) and facts.get("cuts-sha256") == cdigest and facts.get("product") == "yes" and facts.get("exact") == ("yes" if not acc else "no") and facts["names"] == names,
         "cuts.txt beside it says the cuts that are in: %d (the exact library's and %s), digest %s..." % (len(names), ", ".join(acc) or "no picture cut", cdigest[:16]), str(facts)[:300])
    bad = C.compare_committed(files2, picture=True)
    need(not bad and gnames2 == acc, "THE HEADERS OF THE PICTURE CUTS ARE THE CUT SOURCE'S: all 18 passes compiled once more from the copy with the accepted cuts applied on top of the exact ones; the %d header(s) that differ from the exact library's are the committed ones of ffx-own\\shaders-picture, byte for byte, and that folder holds nothing else" % len(files2), "; ".join(bad[:3]))
    over = {}
    for src in (C.committed(), C.committed_picture()):
        if os.path.isdir(src): over.update({f: sha(os.path.join(src, f)) for f in os.listdir(src) if f.endswith("_permutations.h")})
    need(over == facts["headers"], "and the headers in the product's copy are ffx-own\\shaders-cut with ffx-own\\shaders-picture laid over it: %d (cuts.txt names each with its SHA-256)" % len(over))
    hcount, hdigest = len(over), hashlib.sha256("".join("%s  %s\n" % (over[f], f) for f in sorted(over)).encode("ascii")).hexdigest()
    ref3 = dict(rec3) if rec3 else {}
    if NEW_CUT:
        ref3 = dict(rec, library="dlssiv-fg.dll", size=str(size), sha256=h); ref3.update({"cuts": str(len(names)), "cuts-sha256": cdigest, "cut-headers": str(hcount), "cut-headers-sha256": hdigest})
        ok("NEW RECORD (the product's): that library, %d bytes, SHA-256 %s, with %d cut(s)" % (size, h, len(names)))
    else:
        need(rec3 is not None and all(rec3[k] == rec[k] for k in R.KEYS if k not in ("library", "size", "sha256")), "the product's record names the toolchain, the dxc, the headers and the manifest of the other records",
             "they differ in: %s" % ", ".join(k for k in R.KEYS if rec3 and k not in ("library", "size", "sha256") and rec3[k] != rec[k]))
        need((rec3["cuts"], rec3["cuts-sha256"], rec3["cut-headers"], rec3["cut-headers-sha256"]) == (str(len(names)), cdigest, str(hcount), hdigest),
             "the cuts and their headers are the product's record's: %s cut(s), digest %s...; %s header(s), digest %s..." % (rec3["cuts"], rec3["cuts-sha256"][:16], rec3["cut-headers"], rec3["cut-headers-sha256"][:16]),
             "this tree: %d cut(s) %s, %d header(s) %s" % (len(names), cdigest, hcount, hdigest))
        why = R.library_refusal(here, rec3, os.path.basename(R.PATH_CUT))
        need(not why, "THE LIBRARY IS THE PRODUCT'S RECORD'S: %s bytes, SHA-256 %s" % (rec3["size"], rec3["sha256"]), why)
    need(prod4 == (h, size), "the same bytes from the second folder (part 8's copy of the inputs, with _CL_=/fp:fast in the environment)", "this tree %s, the copy %s" % (h, prod4[0]))
    same = open(here, "rb").read() == open(exact, "rb").read()
    need(same == (not acc), "with no accepted picture cut the product IS the exact library, byte for byte" if not acc else "with %d picture cut(s) in it is not the exact library's bytes" % len(acc))
    p = pe_read.PE(here)
    own, exported = p.exports()
    imp = p.imports()
    need(p.machine == 0x8664 and p.is_dll() and own == "dlssiv-fg.dll" and exported == SEVEN and not [d for d in imp if RUNTIME.match(d)] and sorted(d.lower() for d in imp) == ["dxgi.dll", "kernel32.dll"]
         and not p.has_resources() and not p.signed() and not [e for e in p.debug_entries() if e[0] == 2] and b"amd_fidelityfx" not in p.d.lower(),
         "it is a 64-bit DLL that calls itself dlssiv-fg.dll, exports the interface's seven functions and nothing else, imports KERNEL32.dll and dxgi.dll alone, carries no resource, no signature and no PDB path, and names no library of AMD's")
    rc, said = probe_fg(here)
    want = 'interface %d, stage %d, AMD "3.1.6" id 0xF600000000C01006' % (3 if "fi-store-apart" in names else 2 if "fi-position" in names else 1, 3 if names else 2)   # (2026-10-17: fi-position, interface 2; 2026-10-18: fi-store-apart, interface 3)
    need(rc == 0 and want in said and said.rstrip().endswith("PASS") and (not names or ("%d cut(s) %s" % (len(names), cdigest[:8])) in said),
         "with no device it says what it is: %s%s" % (want, (", and \"%d cut(s) %s\": the count and the digest's first eight digits" % (len(names), cdigest[:8])) if names else ""), said.strip()[-400:])
    # THE PIN OF ITS PICTURES, AS FAR AS A PROOF WITH NO DEVICE CAN HOLD IT: the file is there and was made for these picture cuts
    pin = read_pin(); adigest = C.digest(acc)
    if NEW_CUT:
        ok("NEW RECORD: the pin of the product's pictures (tools\\ffx-own\\PICTURES-dlssiv-fg.txt) is %s; where the picture cuts changed it is made again ON PURPOSE, on the card: python tools\\test-fg-own\\run.py --ours --new-pictures" % (
            "not there" if pin is None else "for the picture cuts %s" % " ".join(pin.get("picture-cuts-sha256", ["?"]))[:16]))
    elif acc:
        need(pin is not None and pin.get("picture-cuts") == [str(len(acc))] and pin.get("picture-cuts-sha256") == [adigest],
             "THE PRODUCT'S PICTURES ARE PINNED FOR THESE PICTURE CUTS: tools\\ffx-own\\PICTURES-dlssiv-fg.txt names %d picture cut(s) with the digest %s..., the ones that are accepted (the pictures themselves are held on the card: python tools\\test-fg-own\\run.py --ours)" % (len(acc), adigest[:16]),
             "the pin is %s; this tree accepts %d with the digest %s. A picture cut that is wanted is said so by making the pin again: python tools\\test-fg-own\\run.py --ours --new-pictures" % (
                 "not there" if pin is None else "for %s picture cut(s) with the digest %s" % (" ".join(pin.get("picture-cuts", ["?"])), " ".join(pin.get("picture-cuts-sha256", ["?"]))), len(acc), adigest))
    else: need(pin is None, "no picture cut is accepted, and no pin of the product's pictures lies beside the records (the product is the exact library, held to the reference)")
    # the controls
    x0 = os.path.join(tmp, "cuts-exact")
    rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "exact", "-OutDir", x0])
    d0 = os.path.join(x0, "dlssiv-fg.dll")
    control(rc == 0 and os.path.isfile(d0) and sha(d0) == hx and not os.path.isdir(x0 + "-ref") and not os.path.isdir(x0 + "-exact"), "-Cuts exact builds the exact library's bytes (its record's), into the folder it was given: every picture cut has a switch, and the product less its picture cuts IS the library the judge holds to the reference", re.sub(r"\s+", " ", said)[-300:] if rc else "")
    if acc:
        x1 = os.path.join(tmp, "cuts-less-picture")
        rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "-" + acc[0], "-OutDir", x1])
        d1 = os.path.join(x1, "dlssiv-fg.dll")
        control(rc == 0 and os.path.isfile(d1) and sha(d1) not in (h, hx),
                "the product less its first picture cut (-Cuts -%s) is another library than the product and than the exact library: that cut is in the product's bytes" % acc[0], "SHA-256 %s..." % (sha(d1)[:16] if os.path.isfile(d1) else "?"))
    pics = [n for n, s in table if s == "picture"]
    if pics:
        xp = os.path.join(tmp, "cuts-picture-product")
        rc, said = run(PS + [os.path.join(ROOT, "build-ffx-own.ps1"), "-Target", "dlssiv-fg", "-Cuts", "+" + pics[0], "-OutDir", xp])
        said = re.sub(r"\s+", " ", said)
        control(rc != 0 and "CHANGES THE PICTURE" in said and "which nobody accepted" in said and not [f for f in os.listdir(xp) if f.lower().endswith(".dll")],
                "A PICTURE CUT THAT NOBODY ACCEPTED IS STILL REFUSED IN THE PRODUCT: the product's cuts and %s on top (-Cuts +%s) build no library outside a variant" % (pics[0], pics[0]))
        # ... and one that is slipped in by its state alone: the list of a copy says "accepted" where this tree says "picture"
        lp = os.path.join(fourth, "ffx-own", "cuts", "CUTS.txt"); keep = open(lp, "rb").read()
        text = keep.replace(b"\r\n", b"\n").decode("ascii").split("\n")
        k = next(i for i, l in enumerate(text) if l.split() == [pics[0], "picture"])
        moved = text[:k] + text[k + 1:]
        last = max(i for i, l in enumerate(moved) if l.split()[1:] == ["in"] or l.split()[1:] == ["accepted"])
        open(lp, "wb").write("\r\n".join(moved[:last + 1] + [pics[0] + " accepted"] + moved[last + 1:]).encode("ascii"))
        rc, said = run([sys.executable, os.path.join(fourth, "tools", "ffx-own", "cuts.py"), "digest"])
        slipped = said.split()[0] if rc == 0 and said.split() else ""
        control(re.fullmatch(r"[0-9a-f]{64}", slipped) is not None and slipped != cdigest and ("%d cut(s)" % (len(names) + 1)) in said,
                "the same cut marked \"accepted\" in a copy of the list, by nobody's word: the copy's product has %d cuts with the digest %s..., which is not the record's: this proof fails on it until the record is made again on purpose (and the judge until the pin is)" % (len(names) + 1, slipped[:16]))
        # ... and an exact cut that stands behind an accepted one is refused before anything is built
        if acc and xnames:
            text2 = [l for l in text if l.split() != [xnames[-1], "in"]]
            open(lp, "wb").write("\r\n".join(text2 + [xnames[-1] + " in"]).encode("ascii"))
            rc, said = run([sys.executable, os.path.join(fourth, "tools", "ffx-own", "cuts.py"), "digest"])
            control(rc == 1 and ("the exact cut %s (\"in\") stands behind a cut marked \"accepted\"" % xnames[-1]) in said,
                    "an exact cut written behind an accepted picture cut in a copy of the list is refused with a line that names it: the exact cuts are applied first, always")
        open(lp, "wb").write(keep)
    need(pxsha not in (h, hx), "(the judge's one-pixel control of part 9 is neither the product nor the exact library)")
    return here, size, h, ref3


def main():
    global NEW, NEW_FG, NEW_CUT, NEW_EXACT
    if sys.argv[1:] == ["--new-record"]: NEW = True
    elif sys.argv[1:] == ["--new-record-fg"]: NEW_FG = True
    elif sys.argv[1:] == ["--new-record-exact"]: NEW_EXACT = True
    elif sys.argv[1:] == ["--new-record-cut"]: NEW_CUT = True
    elif sys.argv[1:]: raise SystemExit("usage: python tools\\prove-ffx-own.py [--new-record | --new-record-fg | --new-record-exact | --new-record-cut]")
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: everything started here inherits it
    t0 = time.time()
    tmp = tempfile.mkdtemp(prefix="prove-ffx-own-")
    try:
        rec = None if NEW else R.read()
        top, rel, clean = part_tree()
        rows, mcount, mdigest, amd = part_source(tmp, rec)
        files, facts, hdigest = part_headers(tmp, rec)
        part_checkout(tmp, files, top, rel, clean)      # after part 2: the fresh checkout's headers are held to its regeneration
        dll, size, h, tool, third = part_build(tmp, rec)
        part_reads(tmp, rows, facts, third)
        part_probe(tmp, dll)
        # (the second library's record: with --new-record the first is being made, and the second is made by its own run after it)
        rec2 = R.read(R.PATH_FG) if (not NEW and not NEW_FG and os.path.isfile(R.PATH_FG)) else None
        h3 = size3 = ref3 = None
        hx = sizex = refx = None
        if NEW: dll2 = size2 = h2 = None; print("8. the second library: LEFT OUT of a run that makes the first record (its own record is made by --new-record-fg, after this one is committed)")
        else:
            if not NEW_FG and rec2 is None: raise Failed("tools\\ffx-own\\RECORD-dlssiv-fg.txt is not there: the second library has no record (python tools\\prove-ffx-own.py --new-record-fg makes it)")
            dll2, size2, h2, fourth, prod4, exact4 = part_second(tmp, rec, rec2, dll)
            if NEW_FG: print("9, 10. the exact library and the product: LEFT OUT of a run that makes the reference's record (their own records are made by --new-record-exact and then --new-record-cut, each after the one before is committed)")
            else:
                recx = R.read(R.PATH_EXACT) if (not NEW_EXACT and os.path.isfile(R.PATH_EXACT)) else None
                if not NEW_EXACT and recx is None: raise Failed("tools\\ffx-own\\RECORD-dlssiv-fg-exact.txt is not there: the exact library has no record (python tools\\prove-ffx-own.py --new-record-exact makes it)")
                dllx, sizex, hx, refx, files2, gnames2, pxsha = part_exact(tmp, rec, rec2, recx, dll2, fourth, exact4)
                if NEW_EXACT: print("10. the product: LEFT OUT of a run that makes the exact library's record (its own record is made by --new-record-cut, after this one is committed)")
                else:
                    rec3 = R.read(R.PATH_CUT) if (not NEW_CUT and os.path.isfile(R.PATH_CUT)) else None
                    if not NEW_CUT and rec3 is None: raise Failed("tools\\ffx-own\\RECORD-dlssiv-fg-cut.txt is not there: the product has no record (python tools\\prove-ffx-own.py --new-record-cut makes it)")
                    dll3, size3, h3, ref3 = part_product(tmp, rec, rec2, refx, rec3, dll2, dllx, fourth, prod4, files2, gnames2, pxsha)
        part_alt(tmp, h, (h2, hx, h3))
    except Failed as e:
        print("FAIL: " + str(e))
        return 1
    except SystemExit as e:
        if isinstance(e.code, str): print(e.code if e.code.startswith("FAIL") else "FAIL: " + e.code); return 1
        raise
    finally:
        remove(tmp)
    if NEW:
        values = {"library": "ffx-own-fg.dll", "size": size, "sha256": h, "cl": tool["cl"], "link": tool["link"], "windows-sdk": tool["windows-sdk"], "dxc": G.PIN["version"],
                  "headers": 72, "headers-sha256": hdigest, "manifest-rows": mcount, "manifest-sha256": mdigest}
        changed = R.write(values)
        for k, old, new in changed: print("  replaced  %-16s %s  ->  %s" % (k, old if old is not None else "(no record)", new))
        print("RECORD WRITTEN, AND THIS RUN IS NOT A PASS: tools\\ffx-own\\RECORD.txt now names ffx-own-fg.dll, %d bytes, SHA-256 %s, built with cl %s, link %s, Windows SDK %s; %s. "
              "Commit it with what it was made from, then run the proof again." % (size, h, tool["cl"], tool["link"], tool["windows-sdk"],
                                                                           ("%d value(s) replaced" % len(changed)) if changed else "it replaced nothing: the record already said all of it"))
        return 3
    if NEW_EXACT:
        changed = R.write(refx, R.PATH_EXACT)
        for k, old, new in changed: print("  replaced  %-18s %s  ->  %s" % (k, old if old is not None else "(no record)", new))
        print("THE EXACT LIBRARY'S RECORD WRITTEN, AND THIS RUN IS NOT A PASS: tools\\ffx-own\\RECORD-dlssiv-fg-exact.txt now names dlssiv-fg-exact.dll, %d bytes, SHA-256 %s, with %s cut(s), digest %s (the first library and the reference were held to their records as in every run); %s. "
              "Commit it with what it was made from, then make the product's record (--new-record-cut), then run the proof again; and the judge (python tools\\test-fg-own\\run.py --ours) must hold this library to the reference before the cut counts as exact." % (
                  sizex, hx, refx["cuts"], refx["cuts-sha256"], ("%d value(s) replaced" % len(changed)) if changed else "it replaced nothing: the record already said all of it"))
        return 3
    if NEW_CUT:
        changed = R.write(ref3, R.PATH_CUT)
        for k, old, new in changed: print("  replaced  %-18s %s  ->  %s" % (k, old if old is not None else "(no record)", new))
        print("THE PRODUCT'S RECORD WRITTEN, AND THIS RUN IS NOT A PASS: tools\\ffx-own\\RECORD-dlssiv-fg-cut.txt now names dlssiv-fg.dll, %d bytes, SHA-256 %s, with %s cut(s), digest %s (the first library, the reference and the exact library were held to their records as in every run); %s. "
              "Commit it with what it was made from, then run the proof again. If a picture cut came or went, the pin of the product's pictures is made again too, on purpose and on the card: python tools\\test-fg-own\\run.py --ours --new-pictures." % (
                  size3, h3, ref3["cuts"], ref3["cuts-sha256"], ("%d value(s) replaced" % len(changed)) if changed else "it replaced nothing: the record already said all of it"))
        return 3
    if NEW_FG:
        values = dict(rec, library="dlssiv-fg.dll", size=size2, sha256=h2)
        changed = R.write(values, R.PATH_FG)
        for k, old, new in changed: print("  replaced  %-16s %s  ->  %s" % (k, old if old is not None else "(no record)", new))
        print("THE SECOND RECORD WRITTEN, AND THIS RUN IS NOT A PASS: tools\\ffx-own\\RECORD-dlssiv-fg.txt now names dlssiv-fg.dll, %d bytes, SHA-256 %s (the first library was held to RECORD.txt as in every run); %s. "
              "Commit it with what it was made from, then run the proof again." % (size2, h2, ("%d value(s) replaced" % len(changed)) if changed else "it replaced nothing: the record already said all of it"))
        return 3
    print("PASS: ffx-own-fg.dll is the library of the record (%d bytes, SHA-256 %s, cl %s, link %s, Windows SDK %s), built from the source the manifest lists and the record holds (%s), "
          "the same bytes from two folders, and it answers \"3.1.6\" id 0xF600000000C01006 with no device; AND dlssiv-fg-ref.dll, THE REFERENCE, is the library of the second record (%d bytes, SHA-256 %s): the mod's own "
          "interface over the reference library's own objects of AMD's files, with no cut, the same bytes from two folders, seven exports and none of AMD's; AND dlssiv-fg-exact.dll, THE EXACT LIBRARY, is the library of its record "
          "(%d bytes, SHA-256 %s): built beside that reference by the same run, with the %s exact cut(s) of the list (digest %s...) and the %s header(s) they compile to, and no picture cut; AND dlssiv-fg.dll, THE PRODUCT, is the library of its record "
          "(%d bytes, SHA-256 %s): built by the same run from the copy the exact library was compiled from, with the %s cut(s) of the list (digest %s...: the exact ones and the accepted picture cuts) and the %s header(s) they compile to, "
          "the same bytes from two folders, its pictures pinned for those picture cuts, AMD's vendored files untouched; every control failed as it must. %.0f s. "
          "Not proven here, on purpose: the picture (the judge, on a GPU: tools\\test-fg-own\\run.py --ours: the exact library against the reference and AMD's, the product against its pin)." % (
              size, h, tool["cl"], tool["link"], tool["windows-sdk"], "held to GitHub's listing of AMD's tag in this run" if amd else "NOT checked against AMD in this run: no listing file", size2, h2,
              sizex, hx, refx["cuts"], refx["cuts-sha256"][:16], refx["cut-headers"], size3, h3, ref3["cuts"], ref3["cuts-sha256"][:16], ref3["cut-headers"], time.time() - t0))
    return 0


if __name__ == "__main__":
    sys.exit(main())
