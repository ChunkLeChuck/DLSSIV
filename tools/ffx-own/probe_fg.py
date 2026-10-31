r"""THE SECOND LIBRARY, ASKED WITH NO DEVICE (2026-10-10, the own build's stage 2).

    python tools\ffx-own\probe_fg.py <full path of dlssiv-fg.dll>

Loads the library BY ITS FULL PATH into this process (and into no other: tools\prove-ffx-own.py starts this script as
a process of its own, so the file is not held when it has ended) and calls the mod's own interface
(ffx-own\glue\dlssiv_fg.h) as far as it goes without a Direct3D device:
    dlssivFgInfo            what the library says it is: the interface's number, the stage, AMD's version and id
    and the refusals        a description of another size, a null pointer, a create with nothing, a destroy of nothing,
                            and each per-pair call on something that is not a context: each with the interface's own code
No device is made, no window, nothing of AMD's is loaded. It is for libraries this repository built.
The last line is PASS or FAIL; exit code 0 or 1. A library that lacks one of the seven functions is "lacks <name>".
"""
import ctypes, os, sys

E_ARGUMENT, E_CONTEXT = 0x0F000001, 0x0F000005
SEVEN = ["dlssivFgInfo", "dlssivFgCreate", "dlssivFgDestroy", "dlssivFgPrepare", "dlssivFgOpticalFlow", "dlssivFgInterpolate", "dlssivFgStamps"]


class Info(ctypes.Structure):      # DlssivFgInfo
    _fields_ = [("size", ctypes.c_uint32), ("iface", ctypes.c_uint32), ("stage", ctypes.c_uint32), ("caps", ctypes.c_uint32), ("amdId", ctypes.c_uint64),
                ("amdVersion", ctypes.c_char * 16), ("build", ctypes.c_char * 80), ("contextBytes", ctypes.c_uint64), ("aliasableBytes", ctypes.c_uint64)]


def main():
    if len(sys.argv) != 2 or not os.path.isabs(sys.argv[1]): raise SystemExit("usage: probe_fg.py <full path of the library>")
    path = sys.argv[1]
    try: lib = ctypes.WinDLL(path)
    except OSError as e:
        print("FAIL: %s does not load: %s" % (path, e)); return 1
    fn = {}
    for n in SEVEN:
        try: fn[n] = getattr(lib, n)
        except AttributeError:
            print("FAIL: %s lacks %s" % (path, n)); return 1
        fn[n].restype = ctypes.c_uint32
    bad = []

    def want(what, got, value):
        print("  %-78s 0x%08X%s" % (what, got, "" if got == value else "   WANTED 0x%08X" % value))
        if got != value: bad.append(what)

    info = Info(); info.size = ctypes.sizeof(Info)
    fn["dlssivFgInfo"].argtypes = [ctypes.c_void_p, ctypes.c_void_p]
    rc = fn["dlssivFgInfo"](ctypes.byref(info), None)
    want("dlssivFgInfo, a description of its own size (%d bytes), no device" % ctypes.sizeof(Info), rc, 0)
    print('  it says: interface %d, stage %d, AMD "%s" id 0x%X, caps %d, "%s"' % (info.iface, info.stage, info.amdVersion.decode("ascii", "replace"), info.amdId, info.caps, info.build.decode("ascii", "replace")))
    if info.contextBytes or info.aliasableBytes: bad.append("it names a context's bytes with no description of one")
    small = Info(); small.size = 4
    want("dlssivFgInfo, a description of another size", fn["dlssivFgInfo"](ctypes.byref(small), None), E_ARGUMENT)
    want("dlssivFgInfo, a null pointer", fn["dlssivFgInfo"](None, None), E_ARGUMENT)
    ctx = ctypes.c_void_p(0x1234)
    fn["dlssivFgCreate"].argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p]
    want("dlssivFgCreate, no description", fn["dlssivFgCreate"](None, ctypes.byref(ctx), None), E_ARGUMENT)
    if ctx.value: bad.append("a refused create left a context behind")
    want("dlssivFgCreate, nowhere to put the context", fn["dlssivFgCreate"](None, None, None), E_ARGUMENT)
    nothing = ctypes.create_string_buffer(256)          # zeroes: not a context of this library
    for n in ("dlssivFgDestroy", "dlssivFgPrepare", "dlssivFgOpticalFlow", "dlssivFgInterpolate", "dlssivFgStamps"):
        fn[n].argtypes = [ctypes.c_void_p] if n == "dlssivFgDestroy" else [ctypes.c_void_p, ctypes.c_void_p]
        args = (None,) if n == "dlssivFgDestroy" else (None, None)
        want("%s, a null context" % n, fn[n](*args), E_CONTEXT)
        args = (nothing,) if n == "dlssivFgDestroy" else (nothing, None)
        want("%s, 256 zero bytes for a context" % n, fn[n](*args), E_CONTEXT)
    for b in bad: print("FAIL: " + b)
    print("FAIL" if bad else "PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
