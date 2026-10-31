"""Generates equiv_c227.cpp from equiv_c227.template.cpp and the text of
source/DLSS-IV.cpp between its [equiv_c227: name] and [/equiv_c227: name]
markers, verbatim, so the proof runs the selector that ships and not a copy of it.

    python gen_equiv_c227.py && python run_proof.py equiv_c227
"""
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "..", "..", "source", "DLSS-IV.cpp")
TPL = os.path.join(HERE, "equiv_c227.template.cpp")
OUT = os.path.join(HERE, "equiv_c227.cpp")


def region(text, name):
    a = text.find("// [equiv_c227: %s]" % name)
    b = text.find("// [/equiv_c227: %s]" % name)
    if a < 0 or b < 0 or b < a:
        sys.exit("marker pair [equiv_c227: %s] not found in DLSS-IV.cpp" % name)
    a = text.find("\n", a) + 1                   # the line after the opening marker
    b = text.rfind("\n", 0, b) + 1               # up to the line of the closing marker
    body = text[a:b]
    return body.replace("DLSSIV_HOT ", "")


def main():
    src = open(SRC, encoding="utf-8", errors="replace").read()
    tpl = open(TPL, encoding="utf-8").read()
    for name in ("table", "camdiff", "offer", "count", "elect"):
        key = "/*@@%s@@*/" % name.upper()
        if key not in tpl:
            sys.exit("template has no %s" % key)
        tpl = tpl.replace(key, region(src, name))
    # CRLF, AS THE CHECKOUT WRITES IT (2026-09-24). core.autocrlf is true here, so git checks this file out with CRLF and
    # reads a worktree copy with LF as "modified" until it is checked out again (the bytes normalise to the same blob, but
    # git keeps the eol warning state): after every gate run the tree read as dirty and every build after a gate carried
    # "-dirty" in its id, which the kit's installer refuses. Written CRLF, the regenerated file is what the checkout is.
    open(OUT, "w", encoding="utf-8", newline="\r\n").write(tpl)
    print("wrote %s (%d bytes)" % (OUT, len(tpl)))


if __name__ == "__main__":
    main()
