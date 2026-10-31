"""The rule MenuFlagFind (source/DLSS-IV.cpp) uses to find the game's "a menu is up" flag, run against every
GTAIV.exe under a folder, on disk. NOT part of prove-all: it needs the game's executables, which are not in the repo.

    python tools/check-menu-flag.py  "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Grand Theft Auto IV"

The rule: among the sites of the shape  80 3D a a a a 00 74 0D E8 r r r r 84 C0
(cmp byte ptr [a], 0 / jz +0D / call / test al, al) the address named most often wins, if it has at least 4 sites
and three times the runner-up. Expected: 1.0.4.0 00FC3FFF, 1.0.8.0 010FC63F, Complete Edition 011609F6.
THE CONTROL is FusionFix's own pattern for the flag (jz +4B): one site on 1.0.8.0 and NONE on the Complete Edition,
which is why the mod does not use it and votes instead. A build where the vote fails is reported as such and the mod
then masks the pause menu by difference, as it did before the flag was read at all."""
import glob, os, re, struct, sys, collections
root = sys.argv[1] if len(sys.argv) > 1 else r"C:\Program Files (x86)\Steam\steamapps\common\Grand Theft Auto IV"
KNOWN = {0x4A1AE9B0: ("1.0.4.0", 0x00FC3FFF), 0x57C6FB75: ("1.0.8.0", 0x010FC63F), 0x63D3E735: ("Complete Edition", 0x011609F6)}
VOTE = re.compile(rb"\x80\x3D(....)\x00\x74\x0D\xE8....\x84\xC0", re.S)
FF   = re.compile(rb"\x80\x3D(....).\x74\x4B\xE8....\x84\xC0", re.S)
exes = sorted(set(glob.glob(os.path.join(root, "*", "GTAIV.exe")) + glob.glob(os.path.join(root, "GTAIV.exe"))))
if not exes: print("no GTAIV.exe under %s: nothing checked" % root); sys.exit(0)
bad = 0
for e in exes:
    d = open(e, "rb").read()
    pe = struct.unpack_from("<I", d, 0x3C)[0]; optsz = struct.unpack_from("<H", d, pe + 20)[0]; stamp = struct.unpack_from("<I", d, pe + 8)[0]
    base = struct.unpack_from("<I", d, pe + 24 + 28)[0]; size = struct.unpack_from("<I", d, pe + 24 + 56)[0]
    vsz, va, rsz, raw = struct.unpack_from("<IIII", d, pe + 24 + optsz + 8)
    text = d[raw:raw + rsz]
    c = collections.Counter(a for a in (struct.unpack("<I", m.group(1))[0] for m in VOTE.finditer(text)) if base <= a < base + size)
    top = c.most_common(2)
    best, n = top[0] if top else (0, 0); second = top[1][1] if len(top) > 1 else 0
    ok = n >= 4 and n >= 3 * max(second, 1)
    name, want = KNOWN.get(stamp, ("unknown build %08X" % stamp, None))
    right = ok and (want is None or best == want)
    ff = len(FF.findall(text))
    print("%-24s %-18s vote: %08X with %d sites to %d -> %s%s   | FusionFix's pattern: %d site(s)" % (
        os.path.basename(os.path.dirname(e)), name, best, n, second, "accepted" if ok else "REFUSED",
        "" if want is None else (", the known flag" if best == want else ", NOT the known flag %08X" % want), ff))
    if not right: bad += 1
print("MENU FLAG RULE " + ("PASS" if not bad else "FAIL (%d)" % bad))
sys.exit(1 if bad else 0)
