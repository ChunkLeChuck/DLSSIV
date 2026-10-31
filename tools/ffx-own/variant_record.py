r"""THE RECORD OF A VARIANT LIBRARY FOR A LEG IN THE GAME (2026-10-12): the keys of the product's record, written beside a round's report.

A variant library (tools\ffx-own\build_variants.py) is never the product's file and has no record among the product's.
When the owner is to run one in the game, the kit's installer holds the file it is given to a record
(install-kit-1c.ps1 -OwnRecord2), and the rounds of 2026-10-11 and 2026-10-12 each wrote that record by hand
(RECORD-dlssiv-fg-v-env.txt, RECORD-dlssiv-fg-v-ship.txt). This writes the same lines from what the build left:

    library, size, sha256           the variant's file as it lies in obj\dlssiv-fg-var\<variant>
    cl, link, windows-sdk           toolchain.txt beside it
    dxc, headers, headers-sha256, manifest-rows, manifest-sha256    the product's record's (one toolchain, one source)
    cuts, cuts-sha256               cuts.txt beside it
    cut-headers, cut-headers-sha256 the headers cuts.txt lists; the digest is of the lines "<file> <SHA-256>", by file,
                                    each ended by LF (as RECORD-dlssiv-fg-v-ship.txt's is)

    python tools\ffx-own\variant_record.py <variant> <the record's file to write> ["<what it is, one line for the head>"]
    python tools\ffx-own\variant_record.py --check <variant> <a record>      is the built variant that record's? PASS or FAIL
"NEVER THE PRODUCT'S" stands in the head of every record it writes, and "library" is the variant's own file name.
"""
import hashlib, os, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import build_variants as B
import ffx_record as R


def facts(variant):
    dll = B.path_of(variant)
    if not os.path.isfile(dll): raise SystemExit("FAIL: %s is not built (python tools\\ffx-own\\build_variants.py %s)" % (dll, variant))
    folder = os.path.dirname(dll)
    d = open(dll, "rb").read()
    out = {"library": os.path.basename(dll), "size": str(len(d)), "sha256": hashlib.sha256(d).hexdigest()}
    out.update({k: v for k, v in R.read_toolchain(os.path.join(folder, "toolchain.txt")).items() if k in R.TOOLCHAIN})
    rec = R.read(R.PATH_CUT)
    for k in ("dxc", "headers", "headers-sha256", "manifest-rows", "manifest-sha256"): out[k] = rec[k]
    heads = {}
    for l in open(os.path.join(folder, "cuts.txt"), encoding="ascii").read().splitlines():
        f = l.split()
        if f and f[0] in ("cuts", "cuts-sha256"): out[f[0]] = f[1]
        elif f and f[0] == "header" and len(f) == 3: heads[f[2]] = f[1]
    out["cut-headers"] = str(len(heads))
    out["cut-headers-sha256"] = hashlib.sha256("".join("%s %s\n" % (f, heads[f]) for f in sorted(heads)).encode("ascii")).hexdigest()
    if any(out.get(k) is None for k in R.KEYS_CUT): raise SystemExit("FAIL: the build's files beside %s lack %s" % (dll, ", ".join(k for k in R.KEYS_CUT if out.get(k) is None)))
    return out


def read(path):
    out = {}
    for l in open(path, encoding="ascii").read().splitlines():
        f = l.split()
        if len(f) == 2 and not l.startswith("#"): out[f[0]] = f[1]
    return out


def main():
    a = sys.argv[1:]
    if len(a) == 3 and a[0] == "--check":
        got, want = facts(a[1]), read(a[2])
        bad = [k for k in R.KEYS_CUT if got[k] != want.get(k)]
        for k in bad: print("  %-20s the built variant %s, the record %s" % (k, got[k], want.get(k)))
        print("%s is %sthe library of %s" % (B.path_of(a[1]), "" if not bad else "NOT ", a[2])); print("PASS" if not bad else "FAIL")
        return 1 if bad else 0
    if len(a) not in (2, 3) or a[0].startswith("--"): raise SystemExit(__doc__)
    v = facts(a[0])
    cs = dict(B.VARIANTS).get(a[0]) or next((f[2] for f in B.FROZEN if f[0] == a[0]), [])
    head = ["# THE RECORD OF A VARIANT LIBRARY, NEVER THE PRODUCT'S: %s (python tools\\ffx-own\\build_variants.py %s: %s)." % (v["library"], a[0], ", ".join(cs)),
            "# %s" % (a[2] if len(a) == 3 else "For a leg in the game: the kit's installer holds the file it is given to this record (-OwnRecord2)."),
            "# The keys are the product's record's (tools\\ffx-own\\RECORD-dlssiv-fg-cut.txt); tools\\ffx-own\\variant_record.py wrote it from what the",
            "# build left beside the library and says how cut-headers-sha256 is taken. The product's record is not touched by it."]
    open(a[1], "wb").write(("\r\n".join(head + ["%s %s" % (k, v[k]) for k in R.KEYS_CUT]) + "\r\n").encode("ascii"))
    print("wrote %s: %s, %s bytes, SHA-256 %s, %s cuts %s" % (a[1], v["library"], v["size"], v["sha256"], v["cuts"], v["cuts-sha256"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
