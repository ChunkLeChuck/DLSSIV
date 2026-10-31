r"""AMD'S FILES IN external\ffx\ffx_api_2.3.0, FILE BY FILE AGAINST AMD'S TAG (2026-10-06, the own build of AMD's frame
generation, stage 1a; docs\reports\fg-3\own-build-2026-10-06\critic-plan.md, "THE REPRODUCTION").

The folder holds AMD's public MIT source at tag v2.3.0 in the SDK's own layout, and the library the mod builds from it is
only AMD's if every one of those files is the tag's, byte for byte. external\ffx\MANIFEST-ffx_api_2.3.0.txt lists each
file with the id git gives its bytes (sha1 of "blob <size>", a zero byte, and the bytes), which is the id AMD's own
repository shows for that path at the tag. This script computes the ids again from the files on disk. It needs no git,
no network and no copy of AMD's repository.

It fails on: a listed file that is missing, a size or an id that differs, a file in the folder that the manifest does
not list, a path listed twice, a path outside the folder. A file whose bytes are the tag's but for CRLF line endings is
named as that: it means the working tree was checked out before src\DLSS-IV\.gitattributes kept git's line-ending
conversion out of the folder. That is the state of the 8 headers the helper compiles against in any working tree older
than the rule.

THE CURE FOR THOSE 8, AND THE ONE THAT IS NOT ONE (corrected after the review of 2026-10-06, finding R1.4). In a working
tree that has stood for a while git trusts what it remembers of each file: git status shows NOTHING for the 8 (so no
build is stamped "-dirty" by them), and "git checkout -- external/ffx/ffx_api_2.3.0" rewrites nothing; neither do
checkout -f, reset --hard or restore. This file said the opposite before: it had been measured in a repository whose
files and index were written in the same second, the one case where git does look. WHAT WORKS, measured by the review
and again here in an aged temp repository (8 failures, then none, git status clean): DELETE the folder
external\ffx\ffx_api_2.3.0, and then, from src\DLSS-IV, git checkout -- external/ffx/ffx_api_2.3.0.
(Writing the 8 files again with LF by hand also passes this check, but git then shows them as modified, with an empty
diff, until they are added again: it compares the size it remembers before it compares anything else.)

    python tools\ffx-own\ffx_manifest.py                     the check: one line a failure, then PASS or FAIL; exit code 0 or 1
    python tools\ffx-own\ffx_manifest.py --root <folder>     the same on another copy of external\ffx (the proof's control)
    python tools\ffx-own\ffx_manifest.py --against <file>    as well: every row is in GitHub's own listing of the tag's tree,
                                                             with that path, git blob id and size. <file> is a saved copy of
                                                             the answer to LISTING_URL below (text; this script fetches nothing)
    python tools\ffx-own\ffx_manifest.py --against <clone>   as well: every id in the manifest is the id AMD's repository has
                                                             for that path at the commit the manifest names. <clone> is a
                                                             local clone of the SDK; it is read with lazy fetching off
    python tools\ffx-own\ffx_manifest.py --vendor <clone>    HOW THE FOLDER WAS MADE, kept for the next AMD tag: writes each
                                                             listed file from the clone's blob (no line-ending conversion)
                                                             and rewrites the manifest's ids and sizes from the clone's tree
"""
import hashlib, os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FFX = os.path.join(ROOT, "external", "ffx")
MANIFEST = "MANIFEST-ffx_api_2.3.0.txt"
FOLDER = "ffx_api_2.3.0"
USES = ("own", "helper", "notice", "record")


LONG_ROOT = ("If several are missing: the kit's longest path is 163 characters from the repository's root, and under a root longer than about 95 characters git cannot write it "
             "(Windows allows 260) unless core.longpaths is true. A shorter root helps, or git config core.longpaths true and a new checkout")


def blob_id(data):
    """git's id of a file's bytes"""
    return hashlib.sha1(b"blob %d\0" % len(data) + data).hexdigest()


def read_manifest(path):
    """-> (head, rows): head has 'tag' and 'commit'; a row is (use, id, size, path at the tag, path under external\\ffx)"""
    head = {}; rows = []
    for n, line in enumerate(open(path, encoding="ascii").read().splitlines(), 1):
        if not line.strip(): continue
        if line.startswith("#"):
            m = re.match(r"^#\s*(tag|commit|tree|repository):\s*(\S+)\s*$", line)
            if m: head[m.group(1)] = m.group(2)
            continue
        f = line.split()
        if len(f) != 5 or f[0] not in USES or not re.fullmatch(r"[0-9a-f]{40}", f[1]) or not f[2].isdigit():
            raise SystemExit("FAIL: %s line %d is not 'use id size path-at-the-tag path-here': %s" % (os.path.basename(path), n, line))
        rows.append((f[0], f[1], int(f[2]), f[3], f[4]))
    if "tag" not in head or not re.fullmatch(r"[0-9a-f]{40}", head.get("commit", "")):
        raise SystemExit("FAIL: %s names no tag and commit in its head" % os.path.basename(path))
    return head, rows


def check(root=FFX):
    """-> (failures, files, bytes). root is a copy of external\\ffx: the manifest and the folder are read from it."""
    bad = []
    mp = os.path.join(root, MANIFEST)
    if not os.path.exists(mp): return ["%s is not in %s" % (MANIFEST, root)], 0, 0
    head, rows = read_manifest(mp)
    seen = set(); total = 0
    for use, want, size, tagpath, here in rows:
        key = os.path.normcase(os.path.normpath(here))
        if key in seen: bad.append("%s is listed twice" % here); continue
        seen.add(key)
        if not here.startswith(FOLDER + "/") or ".." in here.split("/"):
            bad.append("%s is listed outside %s" % (here, FOLDER)); continue
        p = os.path.join(root, here.replace("/", os.sep))
        if not os.path.isfile(p):
            # the first missing file carries the one cause nobody would guess (the review of 2026-10-06, R3.7)
            bad.append("%s is listed and is not there%s" % (here, "" if any("is listed and is not there" in b for b in bad) else ". " + LONG_ROOT)); continue
        data = open(p, "rb").read(); total += len(data)
        got = blob_id(data)
        if got == want and len(data) == size: continue
        if blob_id(data.replace(b"\r\n", b"\n")) == want:
            bad.append("%s has CRLF line endings (its bytes are the tag's otherwise): this working tree was checked out before "
                       "src\\DLSS-IV\\.gitattributes existed. DELETE the folder external\\ffx\\%s and then, from src\\DLSS-IV: git checkout -- external/ffx/%s "
                       "(the checkout alone rewrites nothing: git trusts what it remembers of the files)" % (here, FOLDER, FOLDER))
        else:
            bad.append("%s is not AMD's file at %s: %d bytes with id %s, the manifest says %d bytes with id %s" % (here, head["tag"], len(data), got, size, want))
    base = os.path.join(root, FOLDER)
    for dp, dn, fn in os.walk(base):
        for f in fn:
            rel = os.path.relpath(os.path.join(dp, f), root)
            if os.path.normcase(os.path.normpath(rel)) not in seen: bad.append("%s is in the folder and not in the manifest" % rel.replace(os.sep, "/"))
    return bad, len(rows), total


def tree_of(clone, commit):
    """{path at the tag: blob id} from a local clone, nothing fetched"""
    env = dict(os.environ, GIT_NO_LAZY_FETCH="1")
    r = subprocess.run(["git", "-C", clone, "ls-tree", "-r", commit], capture_output=True, text=True, env=env)
    if r.returncode != 0: raise SystemExit("FAIL: git ls-tree of %s in %s: %s" % (commit, clone, r.stderr.strip()))
    out = {}
    for l in r.stdout.splitlines():
        meta, path = l.split("\t", 1)
        out[path] = meta.split()[2]
    return out


def against(clone, root=FFX):
    head, rows = read_manifest(os.path.join(root, MANIFEST))
    tree = tree_of(clone, head["commit"])
    bad = []
    for use, want, size, tagpath, here in rows:
        if tagpath not in tree: bad.append("%s is not a path of AMD's tree at %s" % (tagpath, head["commit"]))
        elif tree[tagpath] != want: bad.append("%s: the manifest says %s, AMD's tree at %s says %s" % (tagpath, want, head["commit"][:8], tree[tagpath]))
    return bad, len(rows), head


# WHAT THE MANIFEST IS HELD TO, WHEN SOMEBODY WANTS MORE THAN OUR OWN WORD (the review of 2026-10-06, R1.2: this check
# compares the files with the manifest and the manifest with nothing; a file and its row changed together pass). GitHub
# answers this address with the whole tree of AMD's commit as JSON: every path with its git blob id and size. Saved to
# a file, it is the list against_listing reads. tools\prove-ffx-own.py looks for it at obj\ffx-own\tree-v2.3.0.json
# (or where the environment variable DLSSIV_FFX_TREE says) and says "not checked against AMD" when it is not there.
LISTING_URL = "https://api.github.com/repos/GPUOpen-LibrariesAndSDKs/FidelityFX-SDK/git/trees/%s?recursive=1"       # % the manifest's commit


def against_listing(path, root=FFX):
    """the manifest's rows against a saved copy of GitHub's listing -> (failures, rows, files in the listing)"""
    import json
    head, rows = read_manifest(os.path.join(root, MANIFEST))
    try: j = json.load(open(path, encoding="utf-8-sig"))
    except (OSError, ValueError) as e: return ["%s is not a saved listing of GitHub's: %s" % (path, e)], len(rows), 0
    if not isinstance(j, dict) or not isinstance(j.get("tree"), list): return ["%s is not a saved listing of GitHub's: no list named tree in it" % path], len(rows), 0
    bad = []
    if j.get("truncated"): bad.append("the listing says it is truncated: not every path of the tree is in it")
    if j.get("sha") not in (head["commit"], head.get("tree")):
        bad.append("the listing is of %s; the manifest names commit %s, whose tree is %s" % (j.get("sha"), head["commit"], head.get("tree", "not named")))
    at = {e.get("path"): (e.get("sha"), e.get("size")) for e in j["tree"] if isinstance(e, dict) and e.get("type") == "blob"}
    for use, want, size, tagpath, here in rows:
        if tagpath not in at: bad.append("%s is not a file of AMD's tree in GitHub's listing" % tagpath)
        elif at[tagpath] != (want, size): bad.append("%s: the manifest says id %s and %d bytes, GitHub's listing says id %s and %s bytes" % (tagpath, want, size, at[tagpath][0], at[tagpath][1]))
    return bad, len(rows), len(at)


def vendor(clone, root=FFX):
    mp = os.path.join(root, MANIFEST)
    head, rows = read_manifest(mp)
    tree = tree_of(clone, head["commit"])
    env = dict(os.environ, GIT_NO_LAZY_FETCH="1")
    new = {}
    for use, want, size, tagpath, here in rows:
        if tagpath not in tree: raise SystemExit("FAIL: %s is not a path of AMD's tree at %s" % (tagpath, head["commit"]))
        r = subprocess.run(["git", "-C", clone, "cat-file", "blob", tree[tagpath]], capture_output=True, env=env)
        if r.returncode != 0: raise SystemExit("FAIL: the clone does not hold the blob of %s (it is not fetched here): %s" % (tagpath, r.stderr.decode(errors="replace").strip()))
        if blob_id(r.stdout) != tree[tagpath]: raise SystemExit("FAIL: the clone's blob of %s does not hash to its own id" % tagpath)
        p = os.path.join(root, here.replace("/", os.sep))
        os.makedirs(os.path.dirname(p), exist_ok=True)
        if not (os.path.isfile(p) and open(p, "rb").read() == r.stdout): open(p, "wb").write(r.stdout)
        new[here] = (tree[tagpath], len(r.stdout))
    raw = open(mp, "rb").read()
    eol = "\r\n" if b"\r\n" in raw else "\n"
    out = []
    for line in raw.decode("ascii").splitlines():
        f = line.split()
        if line.startswith("#") or len(f) != 5: out.append(line); continue
        out.append("%-6s %s %7d %s %s" % (f[0], new[f[4]][0], new[f[4]][1], f[3], f[4]))
    open(mp, "wb").write((eol.join(out) + eol).encode("ascii"))
    print("wrote %d files of AMD's at %s (%s) under %s and their ids into %s" % (len(rows), head["tag"], head["commit"][:8], os.path.join(root, FOLDER), MANIFEST))


def main():
    a = sys.argv[1:]
    root = FFX; clone = None; mode = "check"
    while a:
        o = a.pop(0)
        if o == "--root" and a: root = os.path.abspath(a.pop(0))
        elif o in ("--against", "--vendor") and a: mode = o[2:]; clone = os.path.abspath(a.pop(0))
        else: raise SystemExit("usage: ffx_manifest.py [--root <copy of external\\ffx>] [--against <clone> | --vendor <clone>]")
    if mode == "vendor": vendor(clone, root)
    bad, n, total = check(root)
    for w in bad[:40]: print("  " + w)
    if len(bad) > 40: print("  ... and %d more" % (len(bad) - 40))
    head = read_manifest(os.path.join(root, MANIFEST))[0] if os.path.exists(os.path.join(root, MANIFEST)) else {"tag": "?", "commit": "?"}
    print("AMD's files in external\\ffx\\%s: %d listed, %d bytes: %s" % (FOLDER, n, total, ("each is the file at tag %s (commit %s) by its git blob id, and the folder holds nothing else" % (head["tag"], head["commit"][:8])) if not bad else "NOT as listed (%d)" % len(bad)))
    if mode == "against" and not bad and os.path.isfile(clone):
        bad, n, files = against_listing(clone, root)
        for w in bad[:40]: print("  " + w)
        print("the manifest against GitHub's listing of AMD's tree (%s, %d files): %s" % (clone, files, ("all %d rows are in it with that path, git blob id and size" % n) if not bad else "NOT as listed there (%d)" % len(bad)))
    elif mode == "against" and not bad:
        bad, n, head = against(clone, root)
        for w in bad[:40]: print("  " + w)
        print("the manifest against AMD's repository (%s): %s" % (clone, ("all %d ids are the tree's at commit %s" % (n, head["commit"])) if not bad else "NOT the tree's (%d)" % len(bad)))
    print("PASS" if not bad else "FAIL")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
