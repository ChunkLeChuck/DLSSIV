# hangstacks.py -- where is every thread of a frozen GTAIV.exe?
#
#   python tools\hangstacks.py [process name, default GTAIV.exe] > hang.txt
#
# No debugger and nothing to install: it suspends each thread of the 32-bit game
# for an instant, takes its WOW64 register context, reads the top of its stack,
# and prints every stack word that points into a loaded module's code as
# module+offset. Addresses inside DLSS-IV.asi are named from DLSS-IV.map (the
# linker writes it beside the .asi on every build, so the map must be the one of
# the build that is running: the script prints both timestamps). A scan, not an
# unwind: stale return addresses from earlier calls show up too, so read each
# thread top-down and trust the chain that makes sense, the first few above all.
# Read-only apart from the suspend and resume. Run it from a 64-bit Python.
import ctypes as C, ctypes.wintypes as W, sys, os, bisect, re, time

k32 = C.WinDLL('kernel32', use_last_error=True)
psapi = C.WinDLL('psapi', use_last_error=True)
TH32CS_SNAPPROCESS, TH32CS_SNAPTHREAD = 2, 4
PROCESS_QUERY_INFORMATION, PROCESS_VM_READ = 0x400, 0x10
THREAD_ALL = 0x0002 | 0x0008 | 0x0040          # suspend/resume, get context, query information
LIST_MODULES_32BIT = 1
WOW64_CONTEXT_FULL = 0x00010007

class PROCESSENTRY32(C.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('pid', W.DWORD), ('heap', C.c_void_p), ('mod', W.DWORD),
                ('threads', W.DWORD), ('ppid', W.DWORD), ('pri', W.LONG), ('flags', W.DWORD), ('exe', C.c_char * 260)]
class THREADENTRY32(C.Structure):
    _fields_ = [('dwSize', W.DWORD), ('cntUsage', W.DWORD), ('tid', W.DWORD), ('pid', W.DWORD), ('basePri', W.LONG),
                ('deltaPri', W.LONG), ('flags', W.DWORD)]
class WOW64_FLOATING_SAVE_AREA(C.Structure):
    _fields_ = [('cw', W.DWORD), ('sw', W.DWORD), ('tw', W.DWORD), ('eo', W.DWORD), ('es', W.DWORD), ('do', W.DWORD),
                ('ds', W.DWORD), ('reg', C.c_byte * 80), ('cr0', W.DWORD)]
class WOW64_CONTEXT(C.Structure):
    _fields_ = [('ContextFlags', W.DWORD), ('Dr0', W.DWORD), ('Dr1', W.DWORD), ('Dr2', W.DWORD), ('Dr3', W.DWORD),
                ('Dr6', W.DWORD), ('Dr7', W.DWORD), ('FloatSave', WOW64_FLOATING_SAVE_AREA), ('SegGs', W.DWORD),
                ('SegFs', W.DWORD), ('SegEs', W.DWORD), ('SegDs', W.DWORD), ('Edi', W.DWORD), ('Esi', W.DWORD),
                ('Ebx', W.DWORD), ('Edx', W.DWORD), ('Ecx', W.DWORD), ('Eax', W.DWORD), ('Ebp', W.DWORD),
                ('Eip', W.DWORD), ('SegCs', W.DWORD), ('EFlags', W.DWORD), ('Esp', W.DWORD), ('SegSs', W.DWORD),
                ('Ext', C.c_byte * 512)]
class MODULEINFO(C.Structure):
    _fields_ = [('base', C.c_void_p), ('size', W.DWORD), ('entry', C.c_void_p)]

k32.CreateToolhelp32Snapshot.restype = W.HANDLE
k32.OpenProcess.restype = W.HANDLE
k32.OpenThread.restype = W.HANDLE
k32.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
psapi.EnumProcessModulesEx.argtypes = [W.HANDLE, C.POINTER(C.c_void_p), W.DWORD, C.POINTER(W.DWORD), W.DWORD]
psapi.GetModuleInformation.argtypes = [W.HANDLE, C.c_void_p, C.POINTER(MODULEINFO), W.DWORD]
psapi.GetModuleFileNameExW.argtypes = [W.HANDLE, C.c_void_p, C.c_wchar_p, W.DWORD]

def find_pid(name):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)
    pe = PROCESSENTRY32(); pe.dwSize = C.sizeof(pe)
    ok = k32.Process32First(snap, C.byref(pe)); pid = 0
    while ok:
        if pe.exe.decode(errors='replace').lower() == name.lower(): pid = pe.pid
        ok = k32.Process32Next(snap, C.byref(pe))
    k32.CloseHandle(snap)
    return pid

def threads_of(pid):
    snap = k32.CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0)
    te = THREADENTRY32(); te.dwSize = C.sizeof(te); out = []
    ok = k32.Thread32First(snap, C.byref(te))
    while ok:
        if te.pid == pid: out.append(te.tid)
        ok = k32.Thread32Next(snap, C.byref(te))
    k32.CloseHandle(snap)
    return out

def load_map(path):
    # " 0001:00001234       ?Name@@...       10002234 f   DLSS-IV.obj"
    base, syms = 0x10000000, []
    for line in open(path, errors='replace'):
        m = re.search(r'Preferred load address is ([0-9A-Fa-f]+)', line)
        if m: base = int(m.group(1), 16)
        m = re.match(r'\s*[0-9A-Fa-f]{4}:[0-9A-Fa-f]{8}\s+(\S+)\s+([0-9A-Fa-f]{8})\s', line)
        if m and int(m.group(2), 16) >= base: syms.append((int(m.group(2), 16) - base, m.group(1)))
    syms.sort()
    return syms

def load_exports(path):
    # The export table of a DLL on disk, as sorted (rva, name): enough to turn
    # ntdll.dll+0x79E1C into NtWaitForSingleObject+0xC. No pefile needed.
    import struct
    try:
        d = open(path, 'rb').read()
        pe = struct.unpack_from('<I', d, 0x3C)[0]
        nsec, optsz = struct.unpack_from('<H', d, pe + 6)[0], struct.unpack_from('<H', d, pe + 20)[0]
        opt = pe + 24
        magic = struct.unpack_from('<H', d, opt)[0]
        ddir = opt + (96 if magic == 0x10B else 112)
        exp_rva = struct.unpack_from('<I', d, ddir)[0]
        secs = [struct.unpack_from('<IIII', d, opt + optsz + i * 40 + 8) for i in range(nsec)]   # vsize, va, rawsize, rawptr
        def off(rva):
            for vs, va, rs, rp in secs:
                if va <= rva < va + max(vs, rs): return rva - va + rp
            return None
        if not exp_rva or off(exp_rva) is None: return []
        e = off(exp_rva)
        nfun, nname = struct.unpack_from('<II', d, e + 20)
        afun, aname, aord = [off(x) for x in struct.unpack_from('<III', d, e + 28)]
        out = []
        for i in range(nname):
            nm = off(struct.unpack_from('<I', d, aname + i * 4)[0])
            o = struct.unpack_from('<H', d, aord + i * 2)[0]
            if nm is None or o >= nfun: continue
            out.append((struct.unpack_from('<I', d, afun + o * 4)[0], d[nm:d.index(bytes(1), nm)].decode(errors='replace')))
        out.sort()
        return out
    except Exception:
        return []

def main():
    name = sys.argv[1] if len(sys.argv) > 1 else 'GTAIV.exe'
    pid = find_pid(name)
    if not pid: print('no process named', name); return 1
    hp = k32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not hp: print('OpenProcess failed', C.get_last_error(), '(run this from an elevated prompt if the game is elevated)'); return 1

    mods = []
    arr = (C.c_void_p * 1024)(); need = W.DWORD()
    psapi.EnumProcessModulesEx(hp, arr, C.sizeof(arr), C.byref(need), LIST_MODULES_32BIT)
    for i in range(min(1024, need.value // C.sizeof(C.c_void_p))):
        mi = MODULEINFO(); buf = C.create_unicode_buffer(520)
        if not psapi.GetModuleInformation(hp, arr[i], C.byref(mi), C.sizeof(mi)): continue
        psapi.GetModuleFileNameExW(hp, arr[i], buf, 520)
        mods.append((mi.base or 0, mi.size, buf.value))
    mods.sort()
    bases = [m[0] for m in mods]

    here = os.path.dirname(os.path.abspath(__file__))
    mapPath = os.path.join(here, '..', 'DLSS-IV.map')
    syms = load_map(mapPath) if os.path.exists(mapPath) else []
    rvas = [s[0] for s in syms]
    asi = next((m for m in mods if m[2].lower().endswith('dlss-iv.asi')), None)
    print('pid %d, %d modules, %d threads; %s' % (pid, len(mods), len(threads_of(pid)), time.strftime('%Y-%m-%d %H:%M:%S')))
    if asi and syms:
        print('DLSS-IV.asi at %08X, file %s; map %s' % (asi[0], time.ctime(os.path.getmtime(asi[2])), time.ctime(os.path.getmtime(mapPath))))

    exports = {}
    def name_of(a):
        i = bisect.bisect_right(bases, a) - 1
        if i < 0 or a >= mods[i][0] + mods[i][1]: return None
        b, _, path = mods[i]
        s = '%s+0x%X' % (os.path.basename(path), a - b)
        if asi and b == asi[0] and syms:
            j = bisect.bisect_right(rvas, a - b) - 1
            if j >= 0: s += '  %s+0x%X' % (syms[j][1], a - b - syms[j][0])
        else:
            if path not in exports:
                # A 32-bit process's System32 is SysWOW64 on disk; psapi reports the unredirected name.
                low = path.lower(); k = low.find('/system32/'.replace('/', os.sep))
                disk = path[:k] + os.sep + 'SysWOW64' + path[k + 9:] if k >= 0 else path
                exports[path] = load_exports(disk if os.path.exists(disk) else path)
            ex = exports[path]
            j = bisect.bisect_right([x[0] for x in ex], a - b) - 1 if ex else -1
            if j >= 0 and a - b - ex[j][0] < 0x2000: s += '  %s+0x%X' % (ex[j][1], a - b - ex[j][0])
        return s

    for tid in threads_of(pid):
        ht = k32.OpenThread(THREAD_ALL, False, tid)
        if not ht: print('\nthread %d: OpenThread failed %d' % (tid, C.get_last_error())); continue
        ctx = WOW64_CONTEXT(); ctx.ContextFlags = WOW64_CONTEXT_FULL
        k32.SuspendThread(ht)
        ok = k32.Wow64GetThreadContext(ht, C.byref(ctx))
        stack = (W.DWORD * 1536)(); got = C.c_size_t()
        if ok: k32.ReadProcessMemory(hp, ctx.Esp, stack, C.sizeof(stack), C.byref(got))
        k32.ResumeThread(ht); k32.CloseHandle(ht)
        if not ok: print('\nthread %d: no WOW64 context (%d)' % (tid, C.get_last_error())); continue
        print('\nthread %d   eip %08X  %s   esp %08X ebp %08X' % (tid, ctx.Eip, name_of(ctx.Eip) or '?', ctx.Esp, ctx.Ebp))
        shown = 0
        for i in range(got.value // 4):
            n = name_of(stack[i])
            if n:
                print('   [esp+%04X] %08X  %s' % (i * 4, stack[i], n)); shown += 1
                if shown >= 40: break
    k32.CloseHandle(hp)
    return 0

if __name__ == '__main__':
    sys.exit(main())
