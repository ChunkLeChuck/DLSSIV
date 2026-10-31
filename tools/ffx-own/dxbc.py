r"""READING A SHADER CONTAINER OF OUR OWN (2026-10-06, the own build of AMD's frame generation, stage 1a).

What tools\ffx-own\gen_shaders.py needs to know about a container dxc has just written for it: its parts, the hash dxc
gave the shader, its shader model, the features it declares, the registers it binds (the PSV0 part), and whether the
container's checksum is valid. It is used on containers this repository compiled itself, and on nothing else.

THE CHECKSUM. A container carries 16 bytes at offset 4 that Direct3D 12 checks before it creates a pipeline: an MD5 of
everything from offset 20 on, with a finish of its own (the bit count goes in at the start of the last block and again,
shifted, at its end; the digest is the raw state). dxc leaves them zero unless dxil.dll has validated the shader, so a
valid checksum is what "signed by dxil.dll" means. It is computed here in plain Python so that the check needs no
library of anyone's.
"""
import struct

_S = [7, 12, 17, 22] * 4 + [5, 9, 14, 20] * 4 + [4, 11, 16, 23] * 4 + [6, 10, 15, 21] * 4
_K = [int(abs(__import__("math").sin(i + 1)) * 2 ** 32) & 0xFFFFFFFF for i in range(64)]


def _md5_block(state, block):
    a, b, c, d = state
    m = struct.unpack("<16I", block)
    for i in range(64):
        if i < 16: f = (b & c) | (~b & d); g = i
        elif i < 32: f = (d & b) | (~d & c); g = (5 * i + 1) % 16
        elif i < 48: f = b ^ c ^ d; g = (3 * i + 5) % 16
        else: f = c ^ (b | (~d & 0xFFFFFFFF)); g = (7 * i) % 16
        f = (f + a + _K[i] + m[g]) & 0xFFFFFFFF
        a, d, c = d, c, b
        b = (b + ((f << _S[i]) | (f >> (32 - _S[i])))) & 0xFFFFFFFF
    return [(state[0] + a) & 0xFFFFFFFF, (state[1] + b) & 0xFFFFFFFF, (state[2] + c) & 0xFFFFFFFF, (state[3] + d) & 0xFFFFFFFF]


def checksum(container):
    """the 16 bytes Direct3D expects at offset 4 of this container"""
    data = container[20:]
    bits = (len(data) * 8) & 0xFFFFFFFF
    full = len(data) & ~63
    state = [0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476]
    for o in range(0, full, 64): state = _md5_block(state, data[o:o + 64])
    last = data[full:]
    tail = struct.pack("<I", (bits >> 2) | 1)
    if len(last) >= 56:
        state = _md5_block(state, last + b"\x80" + b"\0" * (63 - len(last)))
        state = _md5_block(state, struct.pack("<I", bits) + b"\0" * 56 + tail)
    else:
        block = struct.pack("<I", bits) + last
        state = _md5_block(state, block + b"\x80" + b"\0" * (59 - len(block)) + tail)
    return struct.pack("<4I", *state)


def parts(c):
    """[(fourcc, offset of the part's data, size)]"""
    if c[:4] != b"DXBC": raise ValueError("not a DXBC container")
    one, total, n = struct.unpack_from("<III", c, 20)
    if total != len(c): raise ValueError("the container says %d bytes and is %d" % (total, len(c)))
    out = []
    for k in range(n):
        po = struct.unpack_from("<I", c, 32 + 4 * k)[0]
        out.append((c[po:po + 4].decode("latin1"), po + 8, struct.unpack_from("<I", c, po + 4)[0]))
    return out


def part(c, name):
    for f, o, s in parts(c):
        if f == name: return c[o:o + s]
    return None


def signed(c):
    """the checksum at offset 4 is there and is the right one"""
    return c[4:20] != b"\0" * 16 and c[4:20] == checksum(c)


def shader_hash(c):
    """the hash dxc gave the shader (the HASH part), as it names the debug file: 32 hex digits"""
    p = part(c, "HASH")
    return p[4:20].hex() if p and len(p) >= 20 else None


def debug_name(c):
    p = part(c, "ILDN")
    if not p: return None
    flags, n = struct.unpack_from("<HH", p, 0)
    return p[4:4 + n].decode("latin1")


def model(c):
    """('cs', 6, 2) from the DXIL part's program header"""
    p = part(c, "DXIL")
    v = struct.unpack_from("<I", p, 0)[0]
    return ({0: "ps", 1: "vs", 2: "gs", 3: "hs", 4: "ds", 5: "cs", 6: "lib"}.get(v >> 16, "k%d" % (v >> 16)), (v >> 4) & 0xF, v & 0xF)


FEATURES = {0x1: "doubles", 0x2: "compute shaders plus raw and structured buffers", 0x4: "UAVs at every stage", 0x8: "64 UAV slots", 0x10: "minimum precision",
            0x20: "double extensions", 0x40: "shader extensions for 11.1", 0x80: "comparison filtering for level 9", 0x100: "tiled resources", 0x200: "stencil ref",
            0x400: "inner coverage", 0x800: "typed UAV load additional formats", 0x1000: "raster ordered UAVs", 0x2000: "viewport and RT array index from any stage",
            0x4000: "wave level operations", 0x8000: "64-bit integers", 0x10000: "view id", 0x20000: "barycentrics", 0x40000: "native low precision (16-bit types)",
            0x80000: "shading rate", 0x100000: "raytracing tier 1.1", 0x200000: "sampler feedback", 0x400000: "atomic 64-bit on typed resource",
            0x800000: "atomic 64-bit on group shared", 0x1000000: "derivatives in mesh and amplification shaders", 0x2000000: "resource descriptor heap indexing",
            0x4000000: "sampler descriptor heap indexing", 0x8000000: "wave matrix (reserved)", 0x10000000: "atomic 64-bit on descriptor heap resource",
            0x20000000: "advanced texture ops", 0x40000000: "writeable MSAA textures", 0x80000000: "sample compare with gradient or bias",
            0x100000000: "extended command info"}
SIXTEEN_BIT = 0x10 | 0x40000      # minimum precision, native low precision: neither may appear in a shader of ours


def features(c):
    p = part(c, "SFI0")
    return struct.unpack_from("<Q", p, 0)[0] if p else 0


def feature_names(bits):
    return [FEATURES.get(1 << i, "bit %d" % i) for i in range(64) if bits >> i & 1]


ANY_LANES = (0, 0xFFFFFFFF)       # the shader has no [WaveSize] on its entry
WAVE64 = (64, 64)                 # [WaveSize(64)] on its entry: it runs only on a device with 64 lanes


def wave_lanes(c):
    """(least, most) wave lanes the shader's entry expects, from the PSV0 part's runtime info (the two numbers dxc's
    listing prints as Minimum and MaximumExpectedWaveLaneCount). None when the part is older than they are."""
    p = part(c, "PSV0")
    if p is None or struct.unpack_from("<I", p, 0)[0] < 24: return None
    return struct.unpack_from("<II", p, 4 + 16)


PSV_CLASS = {1: "sampler", 2: "cb", 3: "srvTex", 4: "srvBuf", 5: "srvBuf", 6: "uavTex", 7: "uavBuf", 8: "uavBuf", 9: "uavBuf"}


def psv_resources(c):
    """the registers the shader binds, from the PSV0 part: [(class, space, first register, last register)]"""
    p = part(c, "PSV0")
    if p is None: return None
    pos = 4 + struct.unpack_from("<I", p, 0)[0]
    count = struct.unpack_from("<I", p, pos)[0]; pos += 4
    out = []
    if count:
        size = struct.unpack_from("<I", p, pos)[0]; pos += 4
        for i in range(count):
            t, space, lo, hi = struct.unpack_from("<IIII", p, pos + i * size)
            out.append((PSV_CLASS.get(t, "type%d" % t), space, lo, hi))
    return out
