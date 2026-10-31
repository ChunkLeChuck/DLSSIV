r"""AMD'S THREE LIBRARIES, PINNED BY SIZE AND MD5, FOR EVERY DESK TOOL THAT COPIES THEM (2026-10-05, frame generation phase 2,
stage 5; the map's helper-19). external\ffx\*.dll is not committed, so a tool that copied "whatever is there" would prove
whatever is there. The loader and the upscaler are pinned as package.ps1 pins them for a release, and this module checks
that its two pins are the packager's, so the two cannot drift. AMD'S FRAME GENERATION LIBRARY IS NOT PACKAGED (2026-10-25:
a shipped helper runs frame generation on the mod's own library, dlssiv-fg.dll, and neither loads AMD's provider nor asks
for it; host\fg_own.h, "THE SHIPPED HELPER"). It stays a development file, the AMD twin of the kit's legs and of the desk
tools that stage it: its pin is the one external\ffx\VERSION.txt records, which is checked, and package.ps1 must NOT pin it.

    from ffx_pin import copy_pinned, PINS
    copy_pinned("amd_fidelityfx_framegeneration_dx12.dll", folder)      raises SystemExit with a FAIL line on any mismatch

    python tools\ffx_pin.py            checks the three files in external\ffx and the two cross-checks; exit code 0 = all hold
"""
import hashlib, os, re, shutil, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FFX = os.path.join(ROOT, "external", "ffx")
LOADER, UPSCALER, FRAMEGEN = "amd_fidelityfx_loader_dx12.dll", "amd_fidelityfx_upscaler_dx12.dll", "amd_fidelityfx_framegeneration_dx12.dll"
PINS = {
    LOADER:   (39184,    "08561837C75E2F9374E48B2320F473B2"),
    UPSCALER: (57664272, "C9B73B38F9F2744C7096DE53429DB6EB"),
    FRAMEGEN: (40085776, "6A12BC4DC82AE11E81C37930B439F6C9"),
}
_checked = {}


def check(name):
    """The file in external\\ffx is the pinned one; "" when it is, the reason when it is not. Hashed once a process."""
    if name in _checked: return _checked[name]
    if name not in PINS: why = "%s has no pin in tools\\ffx_pin.py" % name
    else:
        p = os.path.join(FFX, name); size, md5 = PINS[name]
        if not os.path.exists(p): why = "%s is not in external\\ffx" % name
        elif os.path.getsize(p) != size: why = "%s is %d bytes, the pin says %d" % (name, os.path.getsize(p), size)
        else:
            got = hashlib.md5(open(p, "rb").read()).hexdigest().upper()
            why = "" if got == md5 else "%s has MD5 %s, the pin says %s" % (name, got, md5)
    _checked[name] = why
    return why


def copy_pinned(name, dst):
    why = check(name)
    if why: raise SystemExit("FAIL: %s: a library nobody has checked is not copied into a test" % why)
    shutil.copy2(os.path.join(FFX, name), dst)


def cross_checks():
    """The two pins a release ships are package.ps1's own, and the frame generation library's is VERSION.txt's. Since
    2026-10-25 package.ps1 must hold NO pin of AMD's frame generation library: no package carries it."""
    out = []
    pk = open(os.path.join(ROOT, "package.ps1"), encoding="utf-8", errors="replace").read()
    for name in (LOADER, UPSCALER):
        size, md5 = PINS[name]
        m = re.search(r"name\s*=\s*'%s'\s*;\s*bytes\s*=\s*(\d+)\s*;\s*md5\s*=\s*'([0-9A-Fa-f]{32})'" % re.escape(name), pk)
        if not m: out.append("package.ps1 has no pin for %s in the form this check reads" % name)
        elif (int(m.group(1)), m.group(2).upper()) != (size, md5): out.append("package.ps1 pins %s as %s bytes, MD5 %s; this module says %d, %s" % (name, m.group(1), m.group(2), size, md5))
    if re.search(r"name\s*=\s*'%s'" % re.escape(FRAMEGEN), pk):
        out.append("package.ps1 pins %s: no package carries AMD's frame generation library since 2026-10-25 (the shipped helper runs on dlssiv-fg.dll)" % FRAMEGEN)
    vt = open(os.path.join(FFX, "VERSION.txt"), encoding="utf-8", errors="replace").read()
    size, md5 = PINS[FRAMEGEN]
    m = re.search(r"%s\s+([\d,]+) bytes\s+([0-9A-Fa-f]{32})" % re.escape(FRAMEGEN), vt)
    if not m: out.append("external\\ffx\\VERSION.txt has no line for %s in the form this check reads" % FRAMEGEN)
    elif (int(m.group(1).replace(",", "")), m.group(2).upper()) != (size, md5): out.append("VERSION.txt records %s as %s bytes, MD5 %s; this module says %d, %s" % (FRAMEGEN, m.group(1), m.group(2), size, md5))
    # THE TEST KIT'S PIN (2026-10-05, phase 2: the skeleton leg's kit). tools\kit-1c\t1c.ps1 knows AMD's frame generation library
    # by a size and an MD5 written in that script (it refuses that file under the mod's own library's name): it must be this
    # module's. (The folder's own loader's pin, $fgLoaderPin, went with the legs that loaded AMD's library, round 9's K4, 35849793.)
    kt = open(os.path.join(ROOT, "tools", "kit-1c", "t1c.ps1"), encoding="utf-8", errors="replace").read()
    # (2026-10-29, SDK 2.4.0) and the FSR legs' two pins, the upscaler and the loader the release legs stage from 1c\new-rel
    for var, name in (("fgLibPin", FRAMEGEN), ("fsrLibPin", UPSCALER), ("fsrLoaderPin", LOADER)):
        size, md5 = PINS[name]
        m = re.search(r"\$%s\s*=\s*@\{[^}]*?size\s*=\s*(\d+)\s*;\s*md5\s*=\s*'([0-9A-Fa-f]{32})'" % var, kt)
        if not m: out.append("tools\\kit-1c\\t1c.ps1 has no $%s in the form this check reads" % var)
        elif (int(m.group(1)), m.group(2).upper()) != (size, md5): out.append("t1c.ps1's $%s is %s bytes, MD5 %s; this module says %d, %s for %s" % (var, m.group(1), m.group(2), size, md5, name))
    return out


def main():
    bad = [w for w in (check(n) for n in PINS) if w] + cross_checks()
    for w in bad: print("  " + w)
    print("AMD's three libraries in external\\ffx: %s" % ("each is the pinned file, and the pins are package.ps1's, VERSION.txt's and the test kit's (package.ps1 pinning the loader and the upscaler, and not the frame generation library, which no package carries)" if not bad else "NOT as pinned"))
    print("PASS" if not bad else "FAIL")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
