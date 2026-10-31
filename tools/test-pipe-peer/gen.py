"""THE PIPE'S ONE INSTANCE (the final audit, 2026-09-30, robustHost-2). The helper serves one pipe instance named after the
game's pid, and the default descriptor lets any local account open it for reading: a program that opened it in the seconds
before the game's first connect took the upscaler for the session. The helper now drops a client whose pid is not the
game's (the Windows path, sync=0) and waits again.

The bench runs the BUILT helper (host\\dlssiv-helper.exe, from a copy under %TEMP%) against a small stand-in game: the game
starts the helper with its own pid, a squatter opens the pipe read-only the moment it appears, and then the game connects.
It passes when the helper's log says it dropped the squatter and the game's connect succeeds.

THE CONTROL: the same run with --sync=1, where the check is skipped by design (the Wine path, which takes the pipe as it
always did). The squatter then holds the one instance, the helper reads no hello from it and stops, and the game cannot
connect: the bench must see that, or it proves nothing.

    python tools/test-pipe-peer/gen.py      exit code 0 = the check passed and the control failed as it must
Run it after build-host (prove-all does). The helper opens a D3D12 device, so it needs this machine's graphics card.
"""
import os, sys, subprocess, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
HELPER = os.path.join(ROOT, "host", "dlssiv-helper.exe")
TRANSPORT = os.path.join(ROOT, "source", "transport")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-pipe-peer-%d" % os.getpid())

PROGRAM = r'''#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "feed_ipc.h"
// game.exe <helper> <log> <sync>     the stand-in game
// game.exe squat <pid> <flag>        the squatter: opens the pipe read-only as soon as it exists, holds it 3 s
int main(int argc, char** argv)
{
    char name[128];
    if (argc >= 4 && strcmp(argv[1], "squat") == 0)
    {
        sprintf_s(name, FEED_PIPE_FMT, strtoul(argv[2], nullptr, 10));
        for (int i = 0; i < 600; i++)
        {
            const HANDLE h = CreateFileA(name, GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
            if (h != INVALID_HANDLE_VALUE)
            {
                const HANDLE f = CreateFileA(argv[3], GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, 0, nullptr);
                if (f != INVALID_HANDLE_VALUE) CloseHandle(f);
                Sleep(3000);
                CloseHandle(h);
                return 0;
            }
            Sleep(50);
        }
        return 3;
    }
    if (argc < 4) return 2;
    const DWORD me = GetCurrentProcessId();
    sprintf_s(name, FEED_PIPE_FMT, (unsigned long)me);
    char self[MAX_PATH]; GetModuleFileNameA(nullptr, self, MAX_PATH);
    char flag[MAX_PATH]; sprintf_s(flag, "%s.squatted", argv[2]);
    DeleteFileA(flag);
    char cmd[2048];
    STARTUPINFOA si = { sizeof(si) }; PROCESS_INFORMATION helper = {}, squat = {};
    sprintf_s(cmd, "\"%s\" %lu --log=1 --sync=%s --logpath=\"%s\"", argv[1], (unsigned long)me, argv[3], argv[2]);
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &helper)) { printf("helper did not start %lu\n", GetLastError()); return 4; }
    sprintf_s(cmd, "\"%s\" squat %lu \"%s\"", self, (unsigned long)me, flag);
    if (!CreateProcessA(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &squat)) { printf("squatter did not start %lu\n", GetLastError()); return 4; }
    bool squatted = false;
    for (int i = 0; i < 600 && !squatted; i++) { squatted = GetFileAttributesA(flag) != INVALID_FILE_ATTRIBUTES; if (!squatted) Sleep(50); }
    bool connected = false;
    if (squatted)
        for (int i = 0; i < 200 && !connected; i++)   // 10 s: the squatter lets go after 3
        {
            const HANDLE p = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                         SECURITY_SQOS_PRESENT | SECURITY_IDENTIFICATION, nullptr);
            if (p != INVALID_HANDLE_VALUE) { connected = true; CloseHandle(p); }
            else Sleep(50);
        }
    printf("squatted=%d connected=%d\n", squatted ? 1 : 0, connected ? 1 : 0);
    if (WaitForSingleObject(helper.hProcess, 15000) != WAIT_OBJECT_0) TerminateProcess(helper.hProcess, 9);
    if (WaitForSingleObject(squat.hProcess, 5000) != WAIT_OBJECT_0) TerminateProcess(squat.hProcess, 9);
    return 0;
}
'''


def main():
    if not os.path.exists(HELPER):
        print("FAIL: %s is not built (run build-host first)" % HELPER); return 1
    os.makedirs(OUT, exist_ok=True)
    shutil.copy(HELPER, os.path.join(OUT, "dlssiv-helper.exe"))
    with open(os.path.join(OUT, "game.cpp"), "w") as f: f.write(PROGRAM)
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /EHsc /W3 /std:c++17 /O2 /I"%s" game.cpp /Fe:game.exe' % (vcvars, TRANSPORT),
                       shell=True, capture_output=True, text=True, cwd=OUT)
    if r.returncode != 0 or "warning C" in r.stdout:
        print(r.stdout[-3000:]); print("FAIL: the stand-in game did not compile cleanly"); return 1

    def one(sync):
        log = os.path.join(OUT, "helper-sync%d.log" % sync)
        if os.path.exists(log): os.remove(log)
        g = subprocess.run([os.path.join(OUT, "game.exe"), os.path.join(OUT, "dlssiv-helper.exe"), log, str(sync)],
                           capture_output=True, text=True, cwd=OUT, timeout=120)
        text = open(log, encoding="utf-8", errors="replace").read() if os.path.exists(log) else ""
        return g.stdout.strip(), "opened the pipe: dropped" in text, text

    out, dropped, text = one(0)
    good = out == "squatted=1 connected=1" and dropped
    print("  %s  sync=0: %s, the helper %s the squatter" % ("PASS" if good else "FAIL", out, "dropped" if dropped else "did NOT drop"))
    if not good: print(text[-2000:])
    cout, cdropped, ctext = one(1)
    cgood = cout == "squatted=1 connected=0" and not cdropped
    print("  CONTROL sync=1 (the check skipped by design): %s, %s -> %s" % (cout, "dropped" if cdropped else "not dropped",
          "the squatter kept the pipe and the game could not connect, as it must" if cgood else "NOT what it must be, so the bench proves nothing"))
    if not cgood: print(ctext[-2000:])
    ok = good and cgood
    print("PASS" if ok else "FAIL")
    if ok: shutil.rmtree(OUT, ignore_errors=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
