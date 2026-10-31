r"""THE GAME'S WEB PAGE: THE MOD'S OWN SEARCH RUN OVER ROCKSTAR'S EXECUTABLES AT A DESK (2026-10-13; web.cpp says what is run and
printed; source\fg\fg_web_rule.h is the search; source\fg\fg_web.h what the mod does with the answer).

    python tools\test-fg-web\run.py <GTAIV.exe> [<GTAIV.exe> ...]

Rockstar's files are not in the repository and are only read: name the builds on this machine (a copy is as good).
It builds web.exe in a folder of its own under %TEMP% (dlssiv-test-fg-web), runs it once over the files, and holds:
  - for a build this script knows BY ITS FILE'S SIZE (the three the search was read in on 2026-10-13: 1.0.4.0, 1.0.8.0
    and the Complete Edition), the menu flag and the web page's state at the offsets read that day, each direction's
    counts as read (the shape at one place; -1, 0 and eax written once each), the dword in the file 0, and the
    three controls refused;
  - for any other file: what the search says is printed and NOT judged (a build nobody read: the mod would stand by
    on it only if the search answers, and the line says what it answered).
At least one known build must be named. Exit code 0 = every known build held. The last line is PASS or FAIL.
NOT a step of tools\prove-all.py: it needs files that are not in the repository. No graphics card, no window.
"""
import os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-fg-web")
# size of GTAIV.exe -> (the build, the menu flag's offset, the web page's state's offset), as read on 2026-10-13
KNOWN = {13822600: ("1.0.4.0", 0xBC3FFF, 0x135B678), 15628696: ("1.0.8.0", 0xCFC63F, 0x14CAF10), 17425752: ("the Complete Edition (1.2.0.59)", 0xD609F6, 0x14B6C84)}


def main():
    files = sys.argv[1:]
    if not files: print(__doc__); return 2
    os.makedirs(OUT, exist_ok=True)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    exe = os.path.join(OUT, "web.exe")
    cl = 'call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /MT /W4 /WX /std:c++17 "%s" /Fe:"%s" /Fo:"%s"' % (
        os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat"), os.path.join(HERE, "web.cpp"), exe, os.path.join(OUT, "web.obj"))
    r = subprocess.run(cl, shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0 or not os.path.exists(exe): print(r.stdout[-3000:]); print("FAIL: web.exe did not build"); return 1
    r = subprocess.run([exe] + files, capture_output=True, text=True, errors="replace", creationflags=subprocess.CREATE_NO_WINDOW)
    print(r.stdout.rstrip())
    blocks = re.split(r"(?m)^(?=FILE )", r.stdout)
    known = bad = 0
    for b in blocks:
        m = re.match(r"FILE (.+?): (\d+) bytes;", b)
        if not m: continue
        k = KNOWN.get(int(m.group(2)))
        if not k: print("  (%s: a build this script does not know: printed, not judged)" % m.group(1)); continue
        known += 1
        f = re.search(r"MENU FLAG: found at \+0x([0-9A-F]+) \((\d+) sites to (\d+)\)", b)
        w = re.search(r"WEB PAGE: found at \+0x([0-9A-F]+): found; the shape at (\d+) place\(s\), (\d+) naming another address; written -1 (\d+) time\(s\), 0 (\d+) time\(s\), from eax (\d+) time\(s\); in the file the dword holds (-?\d+)", b)
        ok = (f is not None and int(f.group(1), 16) == k[1] and w is not None and int(w.group(1), 16) == k[2] and [int(x) for x in w.groups()[1:]] == [1, 0, 1, 1, 1, 0]
              and "CONTROLS HELD: 3 of 3" in b and "NOT REFUSED" not in b)
        print("  %s  %s: the menu flag at +0x%X and the web page's state at +0x%X, as read on 2026-10-13; one site, one write of each kind; the three controls refused" % ("ok  " if ok else "FAIL", k[0], k[1], k[2]))
        bad += 0 if ok else 1
    if not known: print("FAIL: none of the named files is a build this script knows (by size: %s)" % ", ".join("%d" % s for s in sorted(KNOWN))); return 1
    print("%d known build(s) held, %d failed" % (known - bad, bad))
    print("FAIL" if bad or r.returncode else "PASS")
    return 1 if bad or r.returncode else 0


if __name__ == "__main__":
    sys.exit(main())
