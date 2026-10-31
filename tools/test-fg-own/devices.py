r"""WHO HOLDS THE CARD: dt_fg_test.exe COUNTS ONLY WHERE IT MAKES A DEVICE (2026-10-11).

ONE GPU USER AT A TIME is every bench's rule, and the cost bench (tools\test-fg-cost\run.py) reads it off the process
list by NAMES: another bench's helper, a fake client, a proof's rig, and dt_fg_test*.exe. That last one is the device
thread's harness, and nearly every run of it is CPU work on a device of its own invention: no Direct3D, no Vulkan, no
card. On 2026-10-11 two other agents ran it in a loop in their own worktrees and the judge of the own build stood
still behind them for twenty minutes, session by session, for a card nobody was using. The round's rule says it as
it is: "dt_fg_test.exe where it makes a device".

narrow(C) makes the cost bench's list say that: a dt_fg_test process counts as a GPU user only when A GRAPHICS LIBRARY
IS LOADED IN IT (Direct3D 9, 10, 11 or 12, DXGI, Vulkan, OpenGL, NVIDIA's or AMD's entry library), read from its own
module list, the 32-bit modules of a 32-bit process too. EVERY OTHER NAME COUNTS AS BEFORE, and a process whose
modules cannot be read (it ended, or it is not this user's) COUNTS: the safe side.

    python tools\test-fg-own\devices.py      every dt_fg_test process now, and what it has loaded
"""
import ctypes, re
from ctypes import wintypes

GRAPHICS = re.compile(r"^(d3d9.*|d3d1[0-2].*|dxgi|dxcore|vulkan-1|opengl32|nvapi(64)?|nvoglv(32|64)|nvwgf2um.*|atidxx(32|64)|amdxx(32|64)|amdvlk(32|64)|igd.*|winevulkan)\.dll$", re.I)
_k32 = ctypes.WinDLL("kernel32", use_last_error=True)
_psapi = ctypes.WinDLL("psapi", use_last_error=True)
_k32.OpenProcess.restype = wintypes.HANDLE
_k32.OpenProcess.argtypes = [wintypes.DWORD, wintypes.BOOL, wintypes.DWORD]
_k32.CloseHandle.argtypes = [wintypes.HANDLE]
_psapi.EnumProcessModulesEx.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.HMODULE), wintypes.DWORD, ctypes.POINTER(wintypes.DWORD), wintypes.DWORD]
_psapi.GetModuleBaseNameW.argtypes = [wintypes.HANDLE, wintypes.HMODULE, wintypes.LPWSTR, wintypes.DWORD]


def modules(pid):
    """the names of every module loaded in a process (32-bit and 64-bit), or None when they cannot be read"""
    h = _k32.OpenProcess(0x0400 | 0x0010, False, pid)          # PROCESS_QUERY_INFORMATION | PROCESS_VM_READ
    if not h: return None
    try:
        arr = (wintypes.HMODULE * 2048)(); need = wintypes.DWORD(0)
        if not _psapi.EnumProcessModulesEx(h, arr, ctypes.sizeof(arr), ctypes.byref(need), 0x03): return None      # LIST_MODULES_ALL
        out = []
        for i in range(min(2048, need.value // ctypes.sizeof(wintypes.HMODULE))):
            buf = ctypes.create_unicode_buffer(260)
            if _psapi.GetModuleBaseNameW(h, arr[i], buf, 260): out.append(buf.value)
        return out or None
    finally:
        _k32.CloseHandle(h)


def makes_device(pid):
    """(does it hold a graphics library, the libraries). Not read: (True, None), the safe side."""
    m = modules(pid)
    if m is None: return True, None
    g = sorted({x.lower() for x in m if GRAPHICS.match(x)})
    return bool(g), g


def narrow(C):
    """the cost bench's other_gpu_users, with a dt_fg_test that holds no graphics library left out"""
    whole = C.other_gpu_users
    if getattr(whole, "narrowed", False): return

    def users():
        out = []
        for u in whole():
            m = re.match(r"dt_fg_test\S* pid (\d+) ", u, re.I)
            if m and not makes_device(int(m.group(1)))[0]: continue
            out.append(u)
        return out
    users.narrowed = True
    C.other_gpu_users = users


if __name__ == "__main__":
    import subprocess
    r = subprocess.run(["tasklist", "/fo", "csv", "/nh"], capture_output=True, text=True, errors="replace")
    n = 0
    for l in r.stdout.splitlines():
        f = [x.strip('"') for x in l.split('","')]
        if len(f) > 1 and f[0].lower().startswith("dt_fg_test"):
            n += 1
            holds, g = makes_device(int(f[1]))
            ms = modules(int(f[1]))
            print("%s pid %s: %s; %s" % (f[0], f[1], "its modules could not be read: COUNTED as a GPU user" if g is None else "%d modules" % len(ms or []),
                                         "" if g is None else ("A GPU USER: " + ", ".join(g)) if holds else "no graphics library loaded: NOT a GPU user"))
    print("%d dt_fg_test process(es)" % n)
    # THE CHECK'S OWN CONTROL: this very process after it has loaded Direct3D 12 must read as one that holds a graphics library
    import os
    before = makes_device(os.getpid())
    ctypes.WinDLL("d3d12.dll")
    after = makes_device(os.getpid())
    print("control: this process before loading d3d12.dll: %s; after: %s" % (before, after))
    print("PASS" if (not before[0] and after[0] and "d3d12.dll" in after[1]) else "FAIL")
