r"""THE CUTS: NAMED, RECORDED CHANGES OF OURS TO AMD'S FRAME GENERATION (2026-10-10, the own build's stage 3, "CUTS THAT
CANNOT CHANGE THE PICTURE"; docs\DECISIONS.md, "PHASE 3: OUR OWN BUILD OF AMD'S FRAME GENERATION", section 4, stage 3;
docs\reports\fg-3\own-build-2026-10-06\critic-plan.md, stage 3).

Stage 2's dlssiv-fg.dll is AMD's source, unmodified, behind the mod's own interface, and every pass of a generated frame
has a measured price. Stage 3 makes a frame cheaper WITHOUT CHANGING ONE BIT OF IT. Every such change is a CUT:

    ffx-own\cuts\CUTS.txt        the list the build reads: one cut a line, by its name, and whether it is IN the product
                                 ("in"), left out ("out"), or a CONTROL that is never in the product ("control");
                                 since 2026-10-11 also "picture" and since 2026-10-12 "accepted" (below, at STATES)
    ffx-own\cuts\<name>.cut     what the cut is: one line of plain words, the shader passes whose compiled headers it
                                 changes, what it means for the wave64 variants (which this PC cannot run), and its
                                 edits: a file of AMD's (by its path in the kit), the text that is there exactly once,
                                 and the text that replaces it

AMD'S VENDORED FILES STAY BYTE FOR BYTE. An edit is made in THE BUILD'S TEMP COPY of external\ffx\ffx_api_2.3.0, never in
the repository (python tools\ffx-own\ffx_manifest.py must say PASS at every commit). A cut whose text is not in the
file exactly once stops the build: nothing is applied "nearly". A cut of our own glue is not an edit: it is a "define"
line, and ffx-own\glue\dlssiv_fg.cpp carries the change under that name (the build writes the names of the cuts that are
in into dlssiv_fg_cuts.h, in its temp copy of the glue).

EVERY CUT HAS A SWITCH AT BUILD TIME: build-ffx-own.ps1 -Target dlssiv-fg -Cuts <names> builds with exactly those
(-Cuts none: with none; a name with "+" before it: the product's cuts and that one; with "-": the product's less that one).

THE SHADER HEADERS OF THE CUTS. A cut of a shader file changes the compiled headers of the passes that include it. The
headers of the product's cuts (the ones marked "in") are committed in a place of their own, ffx-own\shaders-cut, which
holds ONLY the headers that differ from ffx-own\shaders (those stay AMD's source compiled, untouched). Any other set of
cuts is compiled when it is built, by the same pinned dxc and the same checks (tools\ffx-own\gen_shaders.py).

    python tools\ffx-own\cuts.py                 the list, each cut with what it is
    python tools\ffx-own\cuts.py gen             makes ffx-own\shaders-cut from the cuts marked "in" (every pass is
                                                 compiled from the cut copy: a header a cut changes without saying so stops it),
                                                 and ffx-own\shaders-picture from the cuts marked "accepted" on top of them
    python tools\ffx-own\cuts.py --verify        the same into nothing: ffx-own\shaders-cut is what the cuts compile to,
                                                 byte for byte, and holds nothing else; and so is ffx-own\shaders-picture.
                                                 PASS or FAIL, exit code 0 or 1
    python tools\ffx-own\cuts.py digest [spec]   one SHA-256 of a set of cuts (the record holds the product's)
    python tools\ffx-own\cuts.py stage ...       FOR build-ffx-own.ps1: applies a set to the build's temp copy

Nothing here runs on a GPU. Everything runs at BelowNormal priority.
"""
import ctypes, hashlib, os, re, shutil, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import gen_shaders as G

# (2026-10-11) "picture": A CUT THAT CHANGES THE PICTURE. It is never in the product and never in a control: it is built
# only into a VARIANT library of its own name (build-ffx-own.ps1 -Variant <name> -Cuts +<cut> -OutDir <folder>;
# tools\ffx-own\build_variants.py builds them all), for the owner's eye. stage() refuses one in any other build.
STATES = ("in", "out", "control", "picture", "accepted")
# (2026-10-12) "accepted": A CUT THAT CHANGES THE PICTURE AND THAT THE OWNER ACCEPTED IN THE GAME. It is in the product and never in
# the exact library. THE PRODUCT'S PICTURE IS ITS OWN SINCE THAT DAY (docs\reports\fg-3\sitting-2026-10-12b\BRIEF-ship.md), so
# the two kinds of cut are held by two laws:
#     "in"        AN EXACT CUT: it may not change one bit of a generated image. THE EXACT LIBRARY (dlssiv-fg-exact.dll, the
#                 cuts marked "in" and no other; RECORD-dlssiv-fg-exact.txt) is built by every run of the product's build
#                 and is held, pair by pair, to the reference with no cut and to AMD's signed library by the judge.
#     "accepted"  A PICTURE CUT OF THE PRODUCT: the product (dlssiv-fg.dll; RECORD-dlssiv-fg-cut.txt) is the exact library's
#                 source with these applied on top, and its generated pictures on the judge's own set are PINNED by digest
#                 (tools\ffx-own\PICTURES-dlssiv-fg.txt, which only tools\test-fg-own\run.py --ours --new-pictures writes).
#     "picture"   as before: a variant's, nobody accepted it, and stage() refuses it in any build that is not a variant.
# EVERY "in" LINE OF CUTS.txt STANDS BEFORE EVERY "accepted" LINE: the list's order is the order the cuts are applied in, and
# the product is built by applying the accepted cuts to the very copy the exact library was compiled from. An exact cut
# can then never lean on a picture cut's text.
# A SET IS WRITTEN (chosen): "" the product's (the cuts marked "in", then the ones marked "accepted"); "exact" the exact
# library's (the cuts marked "in"); "none"; exact names; "+name" and "-name" from the product's; and "exact,+name,-name"
# from the exact library's (a variant library is built that way: it does not move when the product takes a picture cut).
# (2026-10-12) "frozen-as <name>", the first line of a cut's file: THE CUT IS ANOTHER CUT'S TEXT AS IT STOOD, KEPT UNDER A NEW NAME
# because that name's file has moved on (v-env-1.cut is v-env.cut as the owner ran it in the game on 2026-10-12). The
# digest of a set takes such a cut BY THE NAME AND THE BYTES IT HAD: its SHA-256 is of the file without that one line,
# so a library built from it is, byte for byte, the library that was built then (tools\ffx-own\build_variants.py:
# the frozen libraries, each held to its SHA-256).
FROZEN = "frozen-as "
NAME = re.compile(r"^[a-z][a-z0-9-]{1,39}$")
DEFINE = re.compile(r"^DLSSIV_FG_CUT_[A-Z0-9_]{1,40}$")


class Refused(Exception):
    pass


def folder(root=ROOT): return os.path.join(root, "ffx-own", "cuts")
def committed(root=ROOT): return os.path.join(root, "ffx-own", "shaders-cut")
def committed_picture(root=ROOT): return os.path.join(root, "ffx-own", "shaders-picture")   # the headers of the product that differ from the exact library's


def lf(data):
    return data.replace(b"\r\n", b"\n")


def read_list(root=ROOT):
    """[(name, state)] in the list's order, which is the order the cuts are applied in"""
    p = os.path.join(folder(root), "CUTS.txt")
    if not os.path.isfile(p): raise Refused("%s is not there: the build has no list of cuts" % p)
    out = []
    for n, line in enumerate(lf(open(p, "rb").read()).decode("ascii").split("\n"), 1):
        if not line.strip() or line.startswith("#"): continue
        f = line.split()
        if len(f) != 2 or not NAME.match(f[0]) or f[1] not in STATES or f[0] in [x[0] for x in out]:
            raise Refused("CUTS.txt line %d is not one cut's name and one of %s, once: %s" % (n, ", ".join(STATES), line))
        out.append((f[0], f[1]))
    late = [n for i, (n, s) in enumerate(out) if s == "in" and any(t == "accepted" for _, t in out[:i])]
    if late: raise Refused("CUTS.txt: the exact cut %s (\"in\") stands behind a cut marked \"accepted\": every exact cut is applied before every picture cut of the product (the exact library is the product's own first stage)" % late[0])
    return out


def exact_names(root=ROOT): return [n for n, s in read_list(root) if s == "in"]
def accepted_names(root=ROOT): return [n for n, s in read_list(root) if s == "accepted"]
def product_names(root=ROOT): return exact_names(root) + accepted_names(root)


def read_cut(name, root=ROOT):
    """{what, passes, wave64, defines, edits: [(file, old, new)], sha256} of ffx-own\\cuts\\<name>.cut"""
    p = os.path.join(folder(root), name + ".cut")
    if not os.path.isfile(p): raise Refused("%s is not there: CUTS.txt names a cut that has no file" % p)
    raw = lf(open(p, "rb").read())
    was = None
    if raw.startswith(FROZEN.encode("ascii")):
        first, _, raw = raw.partition(b"\n")
        was = first.decode("ascii")[len(FROZEN):].strip()
        if not NAME.match(was): raise Refused("%s.cut: \"frozen-as\" names a cut: %s" % (name, was))
    lines = raw.decode("ascii").split("\n")
    c = {"name": name, "as": was or name, "what": None, "passes": None, "wave64": None, "defines": [], "edits": [], "sha256": hashlib.sha256(raw).hexdigest()}
    i = 0
    while i < len(lines):
        line = lines[i]; i += 1
        if not line.strip() or line.startswith("#"): continue
        key, _, val = line.partition(" "); val = val.strip()
        if key in ("what", "passes", "wave64"):
            if c[key] is not None or not val: raise Refused("%s.cut: \"%s\" must be there once, with its text" % (name, key))
            c[key] = val
        elif key == "define":
            if not DEFINE.match(val): raise Refused("%s.cut: a define is DLSSIV_FG_CUT_ and capitals: %s" % (name, val))
            c["defines"].append(val)
        elif key == "file":
            if not val or ".." in val or "\\" in val or val.startswith("/"): raise Refused("%s.cut: a file is a path in the kit, with forward slashes: %s" % (name, val))
            part = {}
            for word in ("old", "new"):
                if i >= len(lines) or lines[i] != "--- " + word: raise Refused("%s.cut: after \"file %s\"%s comes the line \"--- %s\"" % (name, val, " and its old text" if word == "new" else "", word))
                i += 1; start = i
                end = "--- new" if word == "old" else "--- end"
                while i < len(lines) and lines[i] != end: i += 1
                if i >= len(lines): raise Refused("%s.cut: the %s text of %s has no line \"%s\" behind it" % (name, word, val, end))
                part[word] = "\n".join(lines[start:i]) + "\n"
                if word == "new": i += 1
            if not part["old"].strip(): raise Refused("%s.cut: an edit of %s with no old text" % (name, val))
            c["edits"].append((val, part["old"].encode("ascii"), part["new"].encode("ascii")))
        else:
            raise Refused("%s.cut: a line that is none of what, passes, wave64, define, file: %s" % (name, line))
    for key in ("what", "passes", "wave64"):
        if c[key] is None: raise Refused("%s.cut lacks its \"%s\" line" % (name, key))
    c["passes"] = [] if c["passes"] == "none" else [x.strip() for x in c["passes"].split(",")]
    known = [p for g in G.GROUPS for p in g["passes"]]
    for p in c["passes"]:
        if p not in known: raise Refused("%s.cut names a pass AMD's source does not have: %s" % (name, p))
    if not c["edits"] and not c["defines"]: raise Refused("%s.cut changes nothing: no edit and no define" % name)
    return c


def chosen(spec, root=ROOT):
    """the names of a set, in the list's order. "": the product's cuts (marked "in", then marked "accepted"). "exact": the
    exact library's (marked "in"). "none": no cut. Else names with commas: a bare name alone builds exactly the named
    ones; "+name" and "-name" add to and take from the product's cuts, or, behind a first word "exact", the exact library's."""
    table = read_list(root)
    names = [n for n, s in table]
    base = [n for n, s in table if s in ("in", "accepted")]
    spec = (spec or "").strip()
    if spec == "": return base
    if spec == "none": return []
    parts = [x.strip() for x in spec.split(",") if x.strip()]
    if parts and parts[0] == "exact":
        base = [n for n, s in table if s == "in"]; parts = parts[1:]
        if [x for x in parts if x[0] not in "+-"]: raise Refused("-Cuts %s: behind \"exact\" every name has + or - before it (the exact library's set, changed)" % spec)
        if not parts: return base
    rel = [x for x in parts if x[0] in "+-"]
    if rel and len(rel) != len(parts): raise Refused("-Cuts %s: either every name has + or - before it (the product's set, changed) or none has (exactly these)" % spec)
    want = set(base) if rel else set()
    for x in parts:
        n = x.lstrip("+-")
        if n not in names: raise Refused("-Cuts %s: CUTS.txt has no cut named %s" % (spec, n))
        if x[0] == "-": want.discard(n)
        else: want.add(n)
    return [n for n in names if n in want]


def digest(names, root=ROOT):
    """one SHA-256 of a set: of the lines "cut <name> <SHA-256 of its file, line endings as LF>", in the set's order"""
    text = "".join("cut %s %s\n" % (c["as"], c["sha256"]) for c in (read_cut(n, root) for n in names))
    return hashlib.sha256(text.encode("ascii")).hexdigest()


def blob_id(data):
    return hashlib.sha1(b"blob %d\0" % len(data) + data).hexdigest()


def apply(kit, names, root=ROOT):
    """the edits of a set, made IN PLACE in a copy of the kit, in the list's order. -> [(cut, file, git blob id before, after)].
    Refused when an old text is not in its file exactly once: then nothing more is written."""
    if os.path.normcase(os.path.abspath(kit)) == os.path.normcase(os.path.abspath(G.KIT)):
        raise Refused("the cuts are applied to a COPY of the kit, never to external\\ffx\\ffx_api_2.3.0 itself")
    done = []
    for n in names:
        c = read_cut(n, root)
        for rel, old, new in c["edits"]:
            p = os.path.join(kit, rel.replace("/", os.sep))
            if not os.path.isfile(p): raise Refused("cut %s: %s is not in the kit" % (n, rel))
            d = open(p, "rb").read()
            if b"\r\n" in d: old, new = old.replace(b"\n", b"\r\n"), new.replace(b"\n", b"\r\n")
            if d.count(old) != 1:
                raise Refused("cut %s: its old text is in %s %d time(s), not once (an earlier cut of the set changed it, the file is not AMD's, or the cut is stale): %s" % (
                    n, rel, d.count(old), old.decode("ascii", "replace").strip().splitlines()[0][:100]))
            e = d.replace(old, new)
            open(p, "wb").write(e)
            done.append((n, rel, blob_id(d), blob_id(e)))
    return done


def passes_of(names, root=ROOT):
    out = []
    for n in names:
        for p in read_cut(n, root)["passes"]:
            if p not in out: out.append(p)
    return out


def cut_headers(kit, names, base, root=ROOT, every=False, over=None):
    """compiles the passes a set says it changes from a kit the set was applied to (every=True: all 18 passes) and
    -> {file name: text} of the headers that DIFFER from the ones in the folder `base` (over: {file name: text} that
    stand in place of that folder's: the headers of the cuts that were applied before this set's)."""
    passes = passes_of(names, root)
    if not passes and not every: return {}
    try: files, facts = G.generate(kit=kit, only=None if every else passes)
    except G.Refused as e: raise Refused("the shader generator on the cut copy: %s" % e)
    out = {}
    for f, text in files.items():
        p = os.path.join(base, f)
        if over and f in over:
            if over[f] != text: out[f] = text
        elif not os.path.isfile(p) or open(p, "rb").read() != text.encode("ascii"): out[f] = text
    said = set(p + v + "_permutations.h" for p in passes for v in ("", "_wave64"))
    stray = sorted(f for f in out if f not in said)
    if stray: raise Refused("the cuts change the headers %s, and no cut of the set names those passes" % ", ".join(stray))
    return out


def glue_header(names, root=ROOT):
    defs = [d for n in names for d in read_cut(n, root)["defines"]]
    lines = ["// dlssiv_fg_cuts.h: MADE BY tools\\ffx-own\\cuts.py FOR ONE BUILD, in that build's temp copy of the glue. Not committed.",
             "// The cuts of this build (ffx-own\\cuts\\CUTS.txt): %s" % (", ".join(names) or "none"),
             "#pragma once",
             "#define DLSSIV_FG_CUTS_COUNT %d" % len(names),
             "#define DLSSIV_FG_CUTS_ID \"%s\"" % digest(names, root)[:8]]
    lines += ["#define %s 1" % d for d in defs]
    return "\r\n".join(lines) + "\r\n"


def committed_of(src, passes, what):
    """{file: bytes} of a folder of committed headers, which may hold only headers of the passes its cuts name"""
    have = sorted(f for f in os.listdir(src) if f.endswith("_permutations.h")) if os.path.isdir(src) else []
    said = sorted(p + v + "_permutations.h" for p in passes for v in ("", "_wave64"))
    if not have or [f for f in have if f not in said]:
        raise Refused("ffx-own\\%s does not hold the headers of %s (it holds %d, and the cuts name %d passes): python tools\\ffx-own\\cuts.py gen" % (os.path.basename(src), what, len(have), len(passes)))
    return {f: open(os.path.join(src, f), "rb").read() for f in have}


def stage(kit, headers, glue, spec, out, root=ROOT, generate=False, variant="", after=""):
    """FOR THE BUILD: applies a set to its temp copy (the kit's files, the headers that change, the glue's list), and
    writes cuts.txt into `out`: what was applied, by names and by bytes. -> the names.
    variant: the name of a VARIANT library (a set with a cut that changes the picture); "" for every other build.
    after: the folder of AN EARLIER STAGE OF THE SAME COPY (its cuts.txt): the cuts it names are in the copy already, they
    must be this set's first ones, and the rest is applied on top. The product's build stages twice: the exact
    library's cuts, and then the accepted picture cuts on the very copy the exact library was compiled from."""
    names = chosen(spec, root)
    state = dict(read_list(root))
    pictures = [n for n in names if state[n] == "picture"]
    accepted = [n for n in names if state[n] == "accepted"]
    if pictures and not variant:
        raise Refused("the set holds %s, which CHANGES THE PICTURE (CUTS.txt: \"picture\") and which nobody accepted: such a cut is built only into a variant library of its own name (build-ffx-own.ps1 -Variant), never into the product, the exact library, a control or a price" % ", ".join(pictures))
    if variant and not pictures and not accepted:
        raise Refused("-Variant %s names a variant library, and no cut of its set changes the picture (CUTS.txt: \"picture\"): it would be an exact library under a variant's name" % variant)
    before, edits, carried = [], [], {}
    if after:
        for l in open(os.path.join(after, "cuts.txt"), encoding="ascii").read().splitlines():
            f = l.split()
            if l.startswith("cut ") and len(f) == 3: before.append(f[1])
            elif l.startswith("edit ") and len(f) == 5: edits.append((f[1], f[4], f[2], f[3]))
            elif l.startswith("header ") and len(f) == 3: carried[f[2]] = f[1]            # the headers the earlier stage put into the copy: still there unless this stage replaces them
        if names[:len(before)] != before: raise Refused("the earlier stage of this copy applied %s, and those are not the first cuts of this set (%s)" % (", ".join(before) or "none", ", ".join(names) or "none"))
    done = edits + apply(kit, names[len(before):], root)
    altered = os.path.isfile(os.path.join(root, "ffx-own", "ALTERED.txt"))
    exact = exact_names(root); base = product_names(root)
    passes = passes_of(names, root)
    how = "none of its cuts changes a shader"
    changed = {}
    if passes:
        if names == exact and not before and not altered and not generate:
            changed = committed_of(committed(root), passes, "the exact cuts")
            how = "the committed headers of the exact cuts (ffx-own\\shaders-cut)"
        elif names == base and before in ([], exact) and not altered and not generate:
            if not before and passes_of(exact, root): changed = committed_of(committed(root), passes_of(exact, root), "the exact cuts")
            if passes_of(accepted, root): changed.update(committed_of(committed_picture(root), passes_of(accepted, root), "the product's picture cuts"))
            how = "the committed headers of the product's cuts (ffx-own\\shaders-cut, and over them ffx-own\\shaders-picture)"
        else:
            changed = {f: t.encode("ascii") for f, t in cut_headers(kit, names, headers, root).items()}
            how = "compiled for this build by dxc %s (not the product's set%s)" % (G.PIN["version"], ", or an altered tree" if altered else "")
        for f, data in changed.items(): open(os.path.join(headers, f), "wb").write(data)
    open(os.path.join(glue, "dlssiv_fg_cuts.h"), "wb").write(glue_header(names, root).encode("ascii"))
    lines = ["cuts %d" % len(names), "cuts-sha256 %s" % digest(names, root), "product %s" % ("yes" if names == base and not altered else "no"), "headers-from %s" % how]
    for n in names:
        c = read_cut(n, root)
        lines.append("cut %s %s" % (n, c["sha256"]))
        lines.append("  what %s" % c["what"])
        lines.append("  wave64 %s" % c["wave64"])
        for d in c["defines"]: lines.append("  define %s" % d)
    for n, rel, a, b in done: lines.append("edit %s %s %s %s" % (n, a, b, rel))
    carried.update({f: hashlib.sha256(changed[f]).hexdigest() for f in changed})
    for f in sorted(carried): lines.append("header %s %s" % (carried[f], f))
    if variant: lines.append("variant %s: A VARIANT, NEVER THE PRODUCT: it changes the picture by %s" % (variant, ", ".join(n for n in names if n in pictures or n in accepted)))
    lines.append("exact %s" % ("yes" if names == exact and not altered else "no"))
    if accepted and not variant: lines.append("picture-cuts %d %s  ACCEPTED BY THE OWNER, the picture is this library's own: %s" % (len(accepted), digest(accepted, root), ", ".join(accepted)))
    open(os.path.join(out, "cuts.txt"), "wb").write(("\n".join(lines) + "\n").encode("ascii"))
    return names


def generate_committed(root=ROOT, picture=False):
    """-> ({file: text} of the headers the exact cuts change, their names). Every pass is compiled from the cut copy.
    picture=True: -> (those, their names, {file: text} of the headers the product's accepted picture cuts change ON TOP
    OF THEM (the headers of the product that differ from the exact library's), their names): all 18 passes once more."""
    names = exact_names(root); more = accepted_names(root)
    tmp = tempfile.mkdtemp(prefix="ffxc-")
    try:
        kit = shutil.copytree(G.KIT, os.path.join(tmp, "k"))
        apply(kit, names, root)
        first = cut_headers(kit, names, G.HEADERS, root, every=True)
        if not picture: return first, names
        second = {}
        if more:
            apply(kit, more, root)
            second = cut_headers(kit, more, G.HEADERS, root, every=True, over=first)
        return first, names, second, more
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def compare_committed(files, root=ROOT, picture=False):
    """the differences between what the exact cuts compile to and ffx-own\\shaders-cut (picture=True: between what the
    product's picture cuts compile to on top of them and ffx-own\\shaders-picture); [] when there is none"""
    src = committed_picture(root) if picture else committed(root); bad = []
    where = "ffx-own\\" + os.path.basename(src)
    have = sorted(f for f in os.listdir(src)) if os.path.isdir(src) else []
    for f in sorted(files):
        if f not in have: bad.append("%s is not in %s" % (f, where))
        elif open(os.path.join(src, f), "rb").read() != files[f].encode("ascii"): bad.append("%s is not what the cut source compiles to" % f)
    for f in have:
        if f not in files: bad.append("%s is in %s and no %s changes that header" % (f, where, "picture cut of the product" if picture else "exact cut"))
    return bad


def main():
    a = sys.argv[1:]
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: dxc inherits it
    try:
        if not a:
            for n, s in read_list():
                c = read_cut(n)
                print("%-8s %-28s %s" % (s, n, c["what"]))
                print("%-8s %-28s passes: %s; edits: %s; defines: %s" % ("", "", ", ".join(c["passes"]) or "none", ", ".join(sorted({e[0].rsplit("/", 1)[-1] for e in c["edits"]})) or "none", ", ".join(c["defines"]) or "none"))
                print("%-8s %-28s wave64: %s" % ("", "", c["wave64"]))
            base = chosen(""); ex = chosen("exact"); acc = accepted_names()
            print("the exact library's cuts: %d (%s); their digest %s" % (len(ex), ", ".join(ex) or "none", digest(ex)))
            print("the product's cuts: those and %d accepted picture cut(s) (%s; the digest of those alone %s): %d; their digest %s" % (len(acc), ", ".join(acc) or "none", digest(acc), len(base), digest(base)))
            return 0
        if a[0] == "digest":
            names = chosen(a[1] if len(a) > 1 else "")
            print("%s  %d cut(s): %s" % (digest(names), len(names), ", ".join(names) or "none")); return 0
        if a[0] in ("gen", "--verify"):
            files, names, files2, names2 = generate_committed(picture=True)
            if a[0] == "gen":
                for dst, fs, ns, state in ((committed(), files, names, "in"), (committed_picture(), files2, names2, "accepted")):
                    if not fs and not os.path.isdir(dst): continue
                    os.makedirs(dst, exist_ok=True)
                    for f in os.listdir(dst):
                        if f not in fs: os.remove(os.path.join(dst, f))
                    for f, t in fs.items(): open(os.path.join(dst, f), "wb").write(t.encode("ascii"))
                    print("wrote %d header(s) into %s for the %d cut(s) marked \"%s\": %s" % (len(fs), dst, len(ns), state, ", ".join(sorted(fs)) or "none"))
                return 0
            bad = compare_committed(files)
            for w in bad[:20]: print("  " + w)
            print("ffx-own\\shaders-cut against the exact library's %d cut(s) compiled again (all 18 passes from the cut copy): %s" % (len(names), "byte-equal, %d header(s), and nothing else there" % len(files) if not bad else "%d DIFFER" % len(bad)))
            bad2 = compare_committed(files2, picture=True)
            for w in bad2[:20]: print("  " + w)
            print("ffx-own\\shaders-picture against the product's %d accepted picture cut(s) compiled again on top of them (all 18 passes): %s" % (len(names2), "byte-equal, %d header(s) that differ from the exact library's, and nothing else there" % len(files2) if not bad2 else "%d DIFFER" % len(bad2)))
            bad += bad2
            print("PASS" if not bad else "FAIL"); return 0 if not bad else 1
        if a[0] == "stage":
            o = {"--root": ROOT, "--cuts": "", "--generate": False, "--variant": "", "--after": ""}
            i = 1
            while i < len(a):
                if a[i] == "--generate": o[a[i]] = True; i += 1
                elif a[i] in ("--kit", "--headers", "--glue", "--cuts", "--out", "--root", "--variant", "--after") and i + 1 < len(a): o[a[i]] = a[i + 1]; i += 2
                else: raise SystemExit("usage: cuts.py stage --kit <copy> --headers <copy> --glue <copy> --out <folder> [--cuts <spec>] [--root <tree>] [--generate] [--variant <name>] [--after <an earlier stage's folder>]")
            names = stage(o["--kit"], o["--headers"], o["--glue"], o["--cuts"], o["--out"], o["--root"], o["--generate"], o["--variant"], o["--after"])
            print("cuts: %d applied to the build's copy: %s" % (len(names), ", ".join(names) or "none")); return 0
        raise SystemExit("usage: cuts.py [gen | --verify | digest [<spec>] | stage ...]")
    except Refused as e:
        print("REFUSED: " + str(e)); print("FAIL"); return 1


if __name__ == "__main__":
    sys.exit(main())
