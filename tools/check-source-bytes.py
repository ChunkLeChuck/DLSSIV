#!/usr/bin/env python3
"""No source file starts with a UTF-8 byte-order mark.

WHY (final audit 2026-09-19, F2.6). DLSS-IV.cpp and DlssIvTransport.h each gained one on 2026-09-18, from an editor or a
PowerShell write, and nobody saw it for a day. A BOM switches MSVC from the ANSI code page to UTF-8 for that ONE file's
narrow literals, so the first non-ASCII character added to a log string would have been encoded differently there than in
every other header; and the project proves no-op changes by byte-comparing builds of exactly those two files. No build
script passes a charset switch, so the rule is: no BOM anywhere. Non-ASCII bytes are REPORTED, not refused.
"""
import os, sys
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXT = (".cpp", ".h", ".hlsl", ".hlsli", ".ps1", ".py", ".cfg")
DIRS = ("source", "host", "tools")


def has_bom(raw):
    return raw.startswith(b"\xef\xbb\xbf")


def main():
    bad, hi, n = [], [], 0
    paths = [os.path.join(ROOT, f) for f in os.listdir(ROOT) if f.endswith((".ps1", ".cfg"))]
    for d in DIRS:
        for base, _, files in os.walk(os.path.join(ROOT, d)):
            paths += [os.path.join(base, f) for f in files if f.endswith(EXT)]
    for p in paths:
        raw = open(p, "rb").read(); n += 1
        rel = os.path.relpath(p, ROOT)
        # PowerShell 5.1 reads a BOM-less .ps1 as ANSI, so a script with non-ASCII text NEEDS its BOM: those are exempt.
        if has_bom(raw) and not (p.endswith(".ps1") and any(b >= 0x80 for b in raw[3:])):
            bad.append(rel)
        elif not has_bom(raw) and any(b >= 0x80 for b in raw):
            hi.append(rel)
    print("%d files read; with a byte-order mark they should not have: %d; without one and not plain ASCII: %d" % (n, len(bad), len(hi)))
    for r in bad:
        print("   BOM: %s" % r)
    for r in hi[:12]:
        print("   (non-ASCII, reported only) %s" % r)
    # THE CONTROL: the test itself on a marked and an unmarked buffer.
    if not has_bom(b"\xef\xbb\xbf// x") or has_bom(b"// x") or n < 40:
        print("FAILED: the control did not bite, or too few files were found to have looked (%d)" % n)
        return 1
    print("FAILED" if bad else "PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
