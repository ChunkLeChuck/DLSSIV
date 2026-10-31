"""THE HELPER'S BUFFERED PIPE READER, TESTED (the optimisation pass of 2026-09-24, HL-1). Compiles
test_pipe_reader.cpp, which includes host/pipe_reader.h as it ships and the wire structs from feed_ipc.h, for x64 as
the helper is built, and runs it on real named pipes: one read a frame against the old shape's two, a split write,
several messages in one write, a burst across the buffer's edge, a close in the middle of a body, and a bad tag.

    python tools/test-pipe-reader/run.py       builds and runs; exit code 0 = every check passed

Needs the same Visual Studio the build uses (vswhere finds it). Writes into %TEMP%\\dlssiv-test-pipe-reader-<pid> (removed when it passes, kept to read when it fails).
"""
import os, sys, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-pipe-reader-%d" % os.getpid())   # per process, as test-ladder's (2026-09-24)


def main():
    os.makedirs(OUT, exist_ok=True)
    src = os.path.join(HERE, "test_pipe_reader.cpp")
    exe = os.path.join(OUT, "test_pipe_reader.exe")
    if os.path.exists(exe):
        os.remove(exe)                     # a stale exe must not pass for a build that failed
    vsw = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), r"Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars64.bat")
    cmd = 'call "%s" >nul 2>&1 && cl /nologo /EHsc /W4 /WX /std:c++17 /O2 "%s" /Fe:test_pipe_reader.exe' % (vcvars, src)
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=OUT)   # a string through the shell: a list would re-quote the paths for cmd
    if r.returncode != 0 or not os.path.exists(exe):
        print(r.stdout); print(r.stderr); print("COMPILE FAILED (exit %d)" % r.returncode); return 2
    r = subprocess.run([exe], capture_output=True, text=True, timeout=60)
    print(r.stdout, end="")
    if r.stderr: print(r.stderr, end="")
    return r.returncode


if __name__ == "__main__":
    rc = main()
    if rc == 0: shutil.rmtree(OUT, ignore_errors=True)   # a pass leaves nothing behind; a failure keeps its files to read
    sys.exit(rc)
