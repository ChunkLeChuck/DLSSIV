r"""THE HEADERS OF A LIBRARY WE BUILT (2026-10-06, the own build of AMD's frame generation, stage 1a).

What tools\prove-ffx-own.py asks of ffx-own-fg.dll without loading it: is it a 64-bit DLL, what does it export, which
DLLs does it import and what from each, does it carry a version resource, a debug directory with a path in it, a
signature. It reads the file as bytes and runs nothing. It is for files this repository built.

    python tools\ffx-own\pe_read.py <file>       prints all of it
"""
import hashlib, struct, sys


class PE:
    def __init__(self, path):
        self.path = path
        self.d = d = open(path, "rb").read()
        if d[:2] != b"MZ": raise ValueError("%s is not a PE file" % path)
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        if d[pe:pe + 4] != b"PE\0\0": raise ValueError("%s is not a PE file" % path)
        self.machine, nsec, self.stamp = struct.unpack_from("<HHI", d, pe + 4)
        self.characteristics = struct.unpack_from("<H", d, pe + 22)[0]
        optsize = struct.unpack_from("<H", d, pe + 20)[0]
        opt = pe + 24
        self.magic = struct.unpack_from("<H", d, opt)[0]
        if self.magic != 0x20B: raise ValueError("%s is not a PE32+ (64-bit) image" % path)
        self.dll_characteristics = struct.unpack_from("<H", d, opt + 70)[0]
        ndir = struct.unpack_from("<I", d, opt + 108)[0]
        self.dirs = [struct.unpack_from("<II", d, opt + 112 + 8 * i) for i in range(ndir)]
        self.sections = []
        for i in range(nsec):
            name, vsize, va, rsize, rptr = struct.unpack_from("<8sIIII", d, opt + optsize + 40 * i)
            self.sections.append((name.rstrip(b"\0").decode("latin1"), va, vsize, rptr, rsize))

    def off(self, rva):
        for name, va, vsize, rptr, rsize in self.sections:
            if va <= rva < va + max(vsize, rsize): return rptr + (rva - va)
        raise ValueError("rva %x is in no section" % rva)

    def cstr(self, rva):
        o = self.off(rva)
        return self.d[o:self.d.index(b"\0", o)].decode("latin1")

    def is_dll(self):
        return bool(self.characteristics & 0x2000)

    def exports(self):
        """(the name the file gives itself, [exported names])"""
        rva, size = self.dirs[0]
        if not rva: return None, []
        o = self.off(rva)
        name_rva = struct.unpack_from("<I", self.d, o + 12)[0]
        nnames, = struct.unpack_from("<I", self.d, o + 24)
        names_rva = struct.unpack_from("<I", self.d, o + 32)[0]
        no = self.off(names_rva)
        return self.cstr(name_rva), sorted(self.cstr(struct.unpack_from("<I", self.d, no + 4 * i)[0]) for i in range(nnames))

    def imports(self):
        """{dll name: [imported names]} from the import table and the delay-load table"""
        out = {}
        rva, size = self.dirs[1]
        if rva:
            o = self.off(rva)
            while True:
                ilt, _, _, name, iat = struct.unpack_from("<IIIII", self.d, o)
                if not name: break
                out.setdefault(self.cstr(name), []).extend(self._thunks(ilt or iat)); o += 20
        rva, size = self.dirs[13] if len(self.dirs) > 13 else (0, 0)
        if rva:
            o = self.off(rva)
            while True:
                attr, name, hmod, iat, int_, _, _, _ = struct.unpack_from("<8I", self.d, o)
                if not name: break
                out.setdefault(self.cstr(name) + " (delay-loaded)", []).extend(self._thunks(int_)); o += 32
        return out

    def _thunks(self, rva):
        names = []; o = self.off(rva)
        while True:
            v = struct.unpack_from("<Q", self.d, o)[0]
            if not v: break
            names.append("ordinal %d" % (v & 0xFFFF) if v >> 63 else self.cstr((v & 0x7FFFFFFF) + 2)); o += 8
        return names

    def has_resources(self):
        return bool(self.dirs[2][0])

    def signed(self):
        return bool(self.dirs[4][0])

    def debug_entries(self):
        """[(type, the text of a CodeView entry's path or '')]"""
        rva, size = self.dirs[6]
        out = []
        if rva:
            o = self.off(rva)
            for i in range(size // 28):
                _, _, _, _, typ, sz, _, ptr = struct.unpack_from("<IIHHIIII", self.d, o + 28 * i)
                text = ""
                if typ == 2 and self.d[ptr:ptr + 4] == b"RSDS": text = self.d[ptr + 24:self.d.index(b"\0", ptr + 24)].decode("latin1")
                out.append((typ, text))
        return out


def main():
    p = PE(sys.argv[1])
    print("%s: %d bytes, SHA-256 %s" % (p.path, len(p.d), hashlib.sha256(p.d).hexdigest()))
    print("  machine 0x%04X, %s, PE date field 0x%08X, DllCharacteristics 0x%04X" % (p.machine, "a DLL" if p.is_dll() else "not a DLL", p.stamp, p.dll_characteristics))
    print("  sections: " + ", ".join("%s %d" % (s[0], s[4]) for s in p.sections))
    own, names = p.exports()
    print("  calls itself %r and exports %d: %s" % (own, len(names), ", ".join(names)))
    for dll, names in sorted(p.imports().items()): print("  imports %d from %s: %s" % (len(names), dll, ", ".join(sorted(names))))
    print("  resources (a version resource would be one): %s; signature: %s; debug entries: %s" % ("yes" if p.has_resources() else "none", "yes" if p.signed() else "none", p.debug_entries() or "none"))


if __name__ == "__main__":
    main()
