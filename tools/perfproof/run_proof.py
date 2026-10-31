"""Compiles and runs an equivalence proof in this folder with the mod's own x86
toolchain (vcvars32), and exits with its exit code.

    python run_proof.py equiv_c227
    python run_proof.py equiv_fgpace -DEQUIV_FGPACE_CONTROL=1          a switch for the compiler
    python run_proof.py equiv_fgpace -DEQUIV_FGPACE_CONTROL=2 -I<dir>  and an include folder
"""
import os, re, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def vcvars32():
    vswhere = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
                           "Microsoft Visual Studio", "Installer", "vswhere.exe")
    root = None
    if os.path.exists(vswhere):
        out = subprocess.run([vswhere, "-latest", "-products", "*", "-requires",
                              "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                              "-property", "installationPath"], capture_output=True, text=True).stdout
        if out.strip():
            root = out.strip().splitlines()[0]
    root = root or r"C:\Program Files\Microsoft Visual Studio\2022\Community"
    return os.path.join(root, "VC", "Auxiliary", "Build", "vcvars32.bat")


def main():
    name = sys.argv[1] if len(sys.argv) > 1 else "equiv_c227"
    # COMPILER SWITCHES (2026-09-26, equiv_fgpace's controls): -DNAME=VALUE goes to cl as /D and -I<dir> as /I; a build with
    # any -D gets its own .exe and .obj (name__<define>), so a control's build never overwrites the proof's own. With neither,
    # the cl line is the one this script always wrote.
    args = sys.argv[2:]
    # A SWITCH THIS SCRIPT CANNOT PASS IS AN ERROR, never dropped (2026-09-26, the verifier's nit 14): a -D whose value is not
    # a plain word (-DX=-1, -DX=1.5) used to fall out of both lists without a word, and a control built without its switch is
    # the real proof. Exit 2, which is neither a proof's verdict (0, 1) nor the compile's (90).
    for a in args:
        if (a.startswith("-D") and not re.fullmatch(r"-D[A-Za-z_]\w*(=\w+)?", a)) or a == "-I":
            print("run_proof.py: cannot pass %r to the compiler (a -D takes NAME or NAME=word, a -I takes a folder); nothing was built" % a)
            print("exit 2")
            sys.exit(2)
    defs = [a for a in args if re.fullmatch(r"-D[A-Za-z_]\w*(=\w+)?", a)]
    incs = [os.path.normpath(a[2:]) for a in args if a.startswith("-I") and len(a) > 2]
    rest = [a for a in args if a not in defs and not a.startswith("-I")]
    out = name + ("__" + "_".join(re.sub(r"\W+", "_", d[2:]) for d in defs).lower() if defs else "")
    switches = "".join(" /D" + d[2:] for d in defs) + "".join(' /I"%s"' % i for i in incs)
    # dt_fg_test COMPILES THE MOD'S GATE BEFORE DXVK'S PRESENT WHOLE (source/transport/present_gate.h, for its case L20, since
    # the fold of 2026-10-10), and that header reads feed_vk.h, which reads Khronos's headers by <vulkan/...>: the one
    # folder they are found in, for that program alone. Every other proof's cl line is the one this script always wrote.
    if name == "dt_fg_test": switches += ' /I"%s"' % os.path.normpath(os.path.join(HERE, "..", "..", "external", "vulkan"))
    bat = os.path.join(os.environ.get("TEMP", HERE), "dlssiv_run_proof.%d.bat" % os.getpid())   # per process (2026-09-24): worktrees running prove-all at once overwrote one another's script, and dt_test failed at random
    with open(bat, "w") as fh:
        fh.write('@echo off\r\ncd /d "%s"\r\ncall "%s" >nul 2>&1\r\n' % (HERE, vcvars32()))
        fh.write('cl /nologo /O2 /EHsc /std:c++17 /W3%s %s.cpp /Fe:%s.exe /Fo:%s.obj\r\n' % (switches, name, out, out))
        # BY FULL PATH: vcvars32 can leave cmd in another folder, and a bare name
        # then reads "not recognized" (exit 9009) after a clean compile.
        # ARGUMENTS AFTER THE NAME GO TO THE PROOF (the gaps audit 2026-09-19): dt_test takes the `threaded` value it runs at,
        # and prove-all could only ever run it at its default of 2, which is not what ships.
        extra = " ".join(a for a in rest if a.replace("-", "").isalnum())
        fh.write('if errorlevel 1 exit /b 90\r\n')
        # dt_test's STAND-IN FOR WINDOWS' COMPATIBILITY LAYER (1.0.8): twin/apphelp.dll, which its `twin` mode loads. Built
        # with every dt_test so the mode never runs a stale one; forward slashes, which cl takes.
        if name == "dt_test" and os.path.exists(os.path.join(HERE, "twin", "apphelp_twin.cpp")):
            fh.write('cl /nologo /O2 /W3 /LD twin/apphelp_twin.cpp /Fe:twin/apphelp.dll /Fo:twin/apphelp_twin.obj'
                     ' /link /IMPLIB:twin/apphelp.lib\r\nif errorlevel 1 exit /b 90\r\n')
        fh.write('"%s" %s\r\nexit /b %%errorlevel%%\r\n' % (os.path.join(HERE, out + ".exe"), extra))
    r = subprocess.run(["cmd.exe", "/c", bat], capture_output=True, text=True)
    try: os.remove(bat)
    except OSError: pass
    # WHAT THE CUT TAKES AWAY IS SAID FIRST (2026-10-12). Only the last 24,000 characters are printed, and dt_fg_test's table of
    # cases alone is longer than that: the gate of 2026-10-11, 17:44, read FAILED and could not name the one case that
    # failed, because its row and its ERROR line stood above the cut. So every line of the part NOT shown that says
    # something went wrong is printed ahead of the tail, as the proof wrote it: a harness's "ERROR: ...", a case's "FAIL: ..."
    # and its row "FAIL <name> ...", "FAILED BECAUSE ...", "HUNG: ...", each from the line's start (a passing check that
    # merely names a verdict it expects, "ok ...: FAIL, for its own reason", is not one). At most 60, each cut to 700
    # characters, with the count of the rest. A proof whose whole output fits prints as it always did.
    keep = 24000                          # 24,000 (2026-09-26): equiv_fgpace prints about 13,000, and 8,000 cut its first streams
    if len(r.stdout) > keep:
        head = r.stdout[:len(r.stdout) - keep]
        head = head[:head.rfind("\n") + 1] if "\n" in head else ""   # (whole lines: the one the cut runs through starts the tail)
        bad = [l for l in head.splitlines() if re.match(r"\s*(ERROR|FAIL|FAILED|HUNG)\b", l)]
        if bad:
            print("run_proof.py: %d character(s) of output stand above the last %d, which follow; in them %d line(s) say what went wrong:"
                  % (len(r.stdout) - keep, keep, len(bad)))
            for l in bad[:60]:
                print(l.rstrip()[:700])
            if len(bad) > 60:
                print("run_proof.py: and %d more such line(s) above the cut" % (len(bad) - 60))
            print("run_proof.py: the last %d characters:" % keep)
    sys.stdout.write(r.stdout[-keep:])
    if r.stderr.strip():
        sys.stdout.write(r.stderr[-3000:])
    print("exit %d%s" % (r.returncode, "  (the compile failed)" if r.returncode == 90 else ""))
    sys.exit(r.returncode)


if __name__ == "__main__":
    main()
