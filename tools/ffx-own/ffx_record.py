r"""THE RECORD OF THE OWN BUILD: WHICH LIBRARY WAS PROVEN, FROM WHAT, WITH WHICH TOOLS (2026-10-06, stage 1a's review,
findings R1.1, R3.4, R1.5 and R1.2; docs\reports\fg-3\own-build-2026-10-06\stage-1a-review).

Until this file existed the proof compared one build with another build of the same run and with nothing else, so any
library the script happened to make was "proven": one built with /fp:fast from the environment, with /O1 in an edited
script, with another dxc argument, or from a source file changed together with its manifest row (the review made all
four and each ended in PASS). tools\ffx-own\RECORD.txt is the committed number they are now held to:

    library, size, sha256           ffx-own-fg.dll as build-ffx-own.ps1 makes it
    cl, link, windows-sdk           the toolchain those bytes belong to; another one makes other bytes
    dxc                             the shader compiler (pinned by hash in gen_shaders.py; said here for the reader)
    headers, headers-sha256         the 72 committed shader headers, one digest (below)
    manifest-rows, manifest-sha256  the rows of external\ffx\MANIFEST-ffx_api_2.3.0.txt, one digest (below)

tools\prove-ffx-own.py fails when any of them is not what it finds, and says both values.

THE TWO DIGESTS, so anybody can compute them again:
    headers-sha256   SHA-256 of the lines "<SHA-256 of the file>  <file name>" for the files of ffx-own\shaders, sorted
                     by name, each line ended by LF.
    manifest-sha256  SHA-256 of the lines "<use> <git blob id> <path at the tag> <path here>" for the manifest's rows,
                     sorted by the path here, each line ended by LF. For the 98 rows of 2026-10-06 it is 7d00456d...,
                     the figure the review computed by itself after holding every row to GitHub's listing of AMD's tag.

A NEW RECORD IS MADE ON PURPOSE, never by hand and never in passing: python tools\prove-ffx-own.py --new-record runs
everything that does not need the record, writes the file from what it built and prints every value it replaced.
That run is not a PASS; the new file is committed, and the next run is.

    python tools\ffx-own\ffx_record.py                    prints the record
    python tools\ffx-own\ffx_record.py <library>          is this file the record's library? PASS or FAIL with both
                                                          hashes; exit code 0 or 1. For anything that stages or loads
                                                          the library: hold the file to the record first.

THE SECOND LIBRARY HAS A RECORD OF ITS OWN (2026-10-07, the own build's stage 2): tools\ffx-own\RECORD-dlssiv-fg.txt names
dlssiv-fg.dll (build-ffx-own.ps1 -Target dlssiv-fg), with the same keys, read and written by the same functions with
PATH_FG. It is made on purpose by python tools\prove-ffx-own.py --new-record-fg, which holds everything of the first
library to RECORD.txt as every run does and writes this one alone.

SINCE THE OWN BUILD'S STAGE 3 (2026-10-10) THERE ARE THREE. RECORD-dlssiv-fg.txt is unchanged and is now the record of
THE REFERENCE: the second library with no cut, built in every run of build-ffx-own.ps1 -Target dlssiv-fg and delivered as
dlssiv-fg-ref.dll (the bytes are stage 2's, and call themselves dlssiv-fg.dll). RECORD-dlssiv-fg-cut.txt (PATH_CUT) is
THE PRODUCT'S: dlssiv-fg.dll with the cuts that are in, and with four keys more (the cuts and the headers they
compile to). It is made by python tools\prove-ffx-own.py --new-record-cut. By the command above a file whose name
begins "dlssiv-fg-ref" is held to the reference's record, any other "dlssiv-fg" to the product's.

SINCE 2026-10-12 THERE ARE FOUR, BECAUSE THE PRODUCT'S PICTURE IS ITS OWN (the owner accepted three cuts that change the picture:
ffx-own\cuts\CUTS.txt, "accepted"; tools\ffx-own\cuts.py, at STATES). RECORD-dlssiv-fg-exact.txt (PATH_EXACT) is THE EXACT
LIBRARY'S: the cuts marked "in" and no picture cut, delivered as dlssiv-fg-exact.dll by every run of the product's
build. Its bytes are what the product was until that day (they call themselves dlssiv-fg.dll inside) and its values
are the ones RECORD-dlssiv-fg-cut.txt held then; ITS "library" LINE IS THE FILE'S OWN NAME, dlssiv-fg-exact.dll, so that
nothing which wants a record of dlssiv-fg.dll (the kit's installer) can be handed this one. It is made by python
tools\prove-ffx-own.py --new-record-exact. RECORD-dlssiv-fg-cut.txt stays THE PRODUCT'S, with the same keys: its cuts are
the exact ones and the accepted ones. THE PRODUCT'S PICTURES have a file of their own beside the records,
PICTURES-dlssiv-fg.txt: tools\test-fg-own\run.py says what it holds, writes it (--ours --new-pictures) and holds the
product to it. By the command above a file whose name begins "dlssiv-fg-exact" is held to the exact library's record.
"""
import hashlib, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
PATH = os.path.join(HERE, "RECORD.txt")
PATH_FG = os.path.join(HERE, "RECORD-dlssiv-fg.txt")      # the second library's as stage 2 built it, with no cut: since stage 3 THE REFERENCE (dlssiv-fg-ref.dll, whose bytes call themselves dlssiv-fg.dll)
PATH_CUT = os.path.join(HERE, "RECORD-dlssiv-fg-cut.txt") # THE PRODUCT'S since stage 3: dlssiv-fg.dll with the cuts that are in (since 2026-10-12: the exact ones and the accepted picture cuts)
PATH_EXACT = os.path.join(HERE, "RECORD-dlssiv-fg-exact.txt")   # THE EXACT LIBRARY'S since 2026-10-12: dlssiv-fg-exact.dll, the cuts marked "in" and no picture cut
PATH_PICTURES = os.path.join(HERE, "PICTURES-dlssiv-fg.txt")    # the product's pinned pictures (tools\test-fg-own\run.py writes and reads it; not a record of this module's keys)
KEYS = ("library", "size", "sha256", "cl", "link", "windows-sdk", "dxc", "headers", "headers-sha256", "manifest-rows", "manifest-sha256")
KEYS_CUT = KEYS + ("cuts", "cuts-sha256", "cut-headers", "cut-headers-sha256")      # the product's record and the exact library's: also the cuts that are in, and the headers they compile to
TOOLCHAIN = ("cl", "link", "windows-sdk")


def keys_of(path):
    return KEYS_CUT if os.path.normcase(os.path.abspath(path)) in (os.path.normcase(PATH_CUT), os.path.normcase(PATH_EXACT)) else KEYS


def read(path=PATH):
    """{key: value}, every key of the record present (KEYS; the product's record: KEYS_CUT); SystemExit with one line otherwise"""
    if not os.path.isfile(path): raise SystemExit("FAIL: %s is not there: the own build has no record to be held to" % path)
    out = {}
    KEYS = keys_of(path)
    for n, line in enumerate(open(path, encoding="ascii").read().splitlines(), 1):
        if not line.strip() or line.startswith("#"): continue
        f = line.split()
        if len(f) != 2 or f[0] not in KEYS or f[0] in out: raise SystemExit("FAIL: %s line %d is not one known key and its value, once: %s" % (os.path.basename(path), n, line))
        out[f[0]] = f[1]
    missing = [k for k in KEYS if k not in out]
    if missing: raise SystemExit("FAIL: %s lacks %s" % (os.path.basename(path), ", ".join(missing)))
    return out


def headers_digest(folder):
    """(count, digest) of the *_permutations.h files of a folder"""
    names = sorted(f for f in os.listdir(folder) if f.endswith("_permutations.h"))
    text = "".join("%s  %s\n" % (hashlib.sha256(open(os.path.join(folder, f), "rb").read()).hexdigest(), f) for f in names)
    return len(names), hashlib.sha256(text.encode("ascii")).hexdigest()


def manifest_digest(rows):
    """(count, digest) of a manifest's rows as ffx_manifest.read_manifest returns them"""
    text = "".join("%s %s %s %s\n" % (use, bid, tagpath, here) for use, bid, size, tagpath, here in sorted(rows, key=lambda r: r[4]))
    return len(rows), hashlib.sha256(text.encode("ascii")).hexdigest()


def library_refusal(path, rec=None, name="RECORD.txt"):
    """"" when this file is the record's library; else the one line that says why not, with both hashes"""
    rec = rec or read()
    if not os.path.isfile(path): return "%s is not there" % path
    d = open(path, "rb").read()
    got = hashlib.sha256(d).hexdigest()
    if got == rec["sha256"] and len(d) == int(rec["size"]): return ""
    return "%s is NOT the library of tools\\ffx-own\\%s: it is %d bytes with SHA-256 %s, the record says %s bytes with SHA-256 %s" % (path, name, len(d), got, rec["size"], rec["sha256"])


def toolchain_refusal(found, rec=None):
    """found is {'cl': ..., 'link': ..., 'windows-sdk': ...} as build-ffx-own.ps1 wrote it. "" when it is the record's"""
    rec = rec or read()
    if all(found.get(k) == rec[k] for k in TOOLCHAIN): return ""
    return ("ANOTHER TOOLCHAIN: this build used %s; tools\\ffx-own\\RECORD.txt is for %s. Another compiler, linker or Windows SDK makes other bytes, so the record is "
            "not for this one and nothing here is proven by it. A new record is made on purpose: python tools\\prove-ffx-own.py --new-record"
            % (", ".join("%s %s" % (k, found.get(k, "?")) for k in TOOLCHAIN), ", ".join("%s %s" % (k, rec[k]) for k in TOOLCHAIN)))


def read_toolchain(path):
    """the toolchain.txt build-ffx-own.ps1 leaves beside the library -> {key: value}"""
    out = {}
    if os.path.isfile(path):
        for line in open(path, encoding="ascii", errors="replace").read().splitlines():
            f = line.split()
            if len(f) == 2: out[f[0]] = f[1]
    return out


HEAD = r"""# THE RECORD OF THE OWN BUILD OF AMD'S FRAME GENERATION: the library that was proven, the toolchain its bytes belong
# to, and the two inputs the proof cannot hold to anything else (the generated headers, the manifest's rows).
# tools\prove-ffx-own.py fails when what it builds or finds is not this. tools\ffx-own\ffx_record.py says how each
# digest is computed. NEVER EDITED BY HAND: python tools\prove-ffx-own.py --new-record writes it and prints what it
# replaced.
"""


HEAD_FG = r"""# THE RECORD OF THE SECOND LIBRARY OF THE OWN BUILD: dlssiv-fg.dll, AMD's frame interpolation, optical flow and backend
# behind the mod's own interface (build-ffx-own.ps1 -Target dlssiv-fg; the own build's stage 2). The same keys as
# RECORD.txt beside it, which names the reference library the same objects link to. tools\prove-ffx-own.py fails when
# what it builds or finds is not this. NEVER EDITED BY HAND: python tools\prove-ffx-own.py --new-record-fg writes it
# and prints what it replaced.
"""


HEAD_CUT = r"""# THE RECORD OF THE PRODUCT OF THE OWN BUILD: dlssiv-fg.dll WITH THE CUTS THAT ARE IN (build-ffx-own.ps1 -Target dlssiv-fg,
# which builds it beside the reference and the exact library). SINCE 2026-10-12 ITS PICTURE IS ITS OWN: its cuts are
# the exact ones (ffx-own\cuts\CUTS.txt: "in") and, on top of them, the picture cuts the owner accepted in the game
# ("accepted"). The keys of the records beside it, and four more: how many cuts are in and one digest of them
# (python tools\ffx-own\cuts.py digest), how many shader headers they change and one digest of those (ffx-own\shaders-cut
# with ffx-own\shaders-picture laid over it, computed as headers-sha256 is). RECORD-dlssiv-fg.txt is the record of the
# library with no cut, the reference; RECORD-dlssiv-fg-exact.txt the exact library's, which the judge holds to that
# reference; PICTURES-dlssiv-fg.txt pins this library's generated pictures. NEVER EDITED BY HAND: python
# tools\prove-ffx-own.py --new-record-cut writes it and prints what it replaced.
"""


HEAD_EXACT = r"""# THE RECORD OF THE EXACT LIBRARY OF THE OWN BUILD: dlssiv-fg-exact.dll, THE EXACT CUTS AND NO PICTURE CUT (ffx-own\cuts\CUTS.txt:
# the cuts marked "in"; build-ffx-own.ps1 -Target dlssiv-fg builds it in every run, between the reference and the
# product, into obj\dlssiv-fg-exact). It is what the product was until 2026-10-12, byte for byte: THE LIBRARY THE JUDGE
# HOLDS, PAIR BY PAIR, TO THE REFERENCE WITH NO CUT AND TO AMD'S SIGNED LIBRARY (tools\test-fg-own\run.py --ours), which is
# the proof that an exact cut is exact. "library" is THE FILE'S NAME; the bytes call themselves dlssiv-fg.dll inside,
# as they did when they were the product. NEVER THE PRODUCT'S FILE: nothing installs or packages it. The keys are
# RECORD-dlssiv-fg-cut.txt's. NEVER EDITED BY HAND: python tools\prove-ffx-own.py --new-record-exact writes it and
# prints what it replaced.
"""


def write(values, path=PATH):
    """writes the record; -> [(key, old, new)] for every value that changed (old is None when there was no record)"""
    old = {}
    if os.path.isfile(path):
        try: old = read(path)
        except SystemExit: old = {}
    KEYS = keys_of(path)
    head = HEAD_EXACT if os.path.normcase(os.path.abspath(path)) == os.path.normcase(PATH_EXACT) else HEAD_CUT if KEYS is KEYS_CUT else HEAD_FG if os.path.normcase(path) == os.path.normcase(PATH_FG) else HEAD
    lines = head.splitlines() + ["%s %s" % (k, values[k]) for k in KEYS]
    eol = "\r\n" if (os.path.isfile(path) and b"\r\n" in open(path, "rb").read()) or not os.path.isfile(path) else "\n"
    open(path, "wb").write((eol.join(lines) + eol).encode("ascii"))
    return [(k, old.get(k), str(values[k])) for k in KEYS if old.get(k) != str(values[k])]


def main():
    base = os.path.basename(sys.argv[1]).lower() if len(sys.argv) == 2 else ""
    which = PATH_FG if base.startswith("dlssiv-fg-ref") else PATH_EXACT if base.startswith("dlssiv-fg-exact") else PATH_CUT if base.startswith("dlssiv-fg") else PATH
    rec = read(which)
    if len(sys.argv) == 1:
        for k in KEYS: print("%-16s %s" % (k, rec[k]))
        for p, what in ((PATH_FG, "the second library's with no cut, the reference"), (PATH_EXACT, "the exact library's: the exact cuts and no picture cut"), (PATH_CUT, "the product's, with the cuts that are in")):
            if os.path.isfile(p):
                print("and %s (%s):" % (what, os.path.basename(p)))
                for k, v in read(p).items(): print("%-18s %s" % (k, v))
        return 0
    if len(sys.argv) != 2: raise SystemExit("usage: ffx_record.py [<library>]")
    why = library_refusal(os.path.abspath(sys.argv[1]), rec, os.path.basename(which))
    print(why if why else "%s is the library of the record: %s bytes, SHA-256 %s" % (os.path.abspath(sys.argv[1]), rec["size"], rec["sha256"]))
    print("PASS" if not why else "FAIL")
    return 0 if not why else 1


if __name__ == "__main__":
    sys.exit(main())
