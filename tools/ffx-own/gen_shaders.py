r"""THE SHADER HEADERS OF THE OWN BUILD OF AMD'S FRAME GENERATION (2026-10-06, stage 1a; grown from the trial's
gen_shaders.py, docs\reports\fg-3\own-build-2026-10-06\trial-build).

AMD's C++ (ffx_frameinterpolation_shaderblobs.cpp, ffx_opticalflow_shaderblobs.cpp) includes 72 headers named
<pass>[_wave64][_16bit]_permutations.h that AMD's tree does not hold: its own build makes them with FidelityFX_SC.exe,
a tool that is not published at tag v2.3.0, neither as a binary nor as source. This script is our stand-in for it. It
compiles the 18 pass shaders of external\ffx\ffx_api_2.3.0 with the Windows SDK's dxc, with the arguments of AMD's two
build scripts (vendored beside the shaders as the record, and compared with ours on every run), reads each shader's
registers from dxc's own listing, drops equal containers and writes the headers. Nothing is downloaded.

THE WHOLE COMMAND LINE IS HELD, not a part of it (the review of 2026-10-06, finding R1.5: an "-O1" added where the
line was put together passed the comparison, because only AMD's half of the line was compared). command() below builds
every token of a dxc run and nothing is added to it anywhere else; on every run that whole line, less the few tokens
written down as ours (the two lists above it say which and why), must be AMD's tokens in AMD's order. What that cannot
see, an edit of this very file's lists, the record sees: tools\ffx-own\RECORD.txt holds one digest of the 72 headers.

    python tools\ffx-own\gen_shaders.py              writes the 72 headers into ffx-own\shaders
    python tools\ffx-own\gen_shaders.py --verify     regenerates them into a temp folder and compares each with the
                                                     committed one, byte for byte; exit code 1 on any difference
    --kit <folder>     another copy of external\ffx\ffx_api_2.3.0 (the proof's controls)
    --out <folder>     write the headers somewhere else (never with --verify)
    --against <folder> with --verify: the headers to hold to the regeneration, instead of ffx-own\shaders
    --reads <file>     also write the list of AMD's files dxc opened, one a line: its git blob id and its path in the kit
    --threads <n>      dxc runs at a time (default 4); everything runs at BelowNormal priority

WHICH DXC. The bytes of a compiled shader follow the compiler's version, so the committed headers verify with one dxc
only, and it is named here: version, and the SHA-256 of dxc.exe, dxcompiler.dll and dxil.dll (PIN below). It is looked
for in the Windows SDKs installed on this PC, or taken from the environment variable DXC_EXE. Another one is refused
with one line and is never started.

WHAT IS COMPILED, AND WHAT IS NOT. Two of AMD's four variants: the base one (cs_6_2, FFX_HALF=0) and the wave64 one
(cs_6_6, with AMD's argument -DFFX_PREFER_WAVE64=[WaveSize(64)]), every permutation of each, as AMD's scripts list
them: none is left out. AMD's C++ picks wave64 on a device that reports 32 to 64 lanes and shader model 6.6, and the
base one everywhere else. THAT ARGUMENT DOES NOT PUT [WaveSize(64)] ON EVERY wave64 SHADER (this file and 18 headers
said so until the review of 2026-10-06, R2.4): it defines a macro, and only the five optical flow passes whose source
uses the macro get the attribute on their entry. The other 18 wave64 containers are cs_6_6 shaders with no wave size
of their own. The script reads it from each container (the two expected lane counts of its PSV0 part, held to what
dxc's listing says of the same shader), prints the real count, and each wave64 header's first line says which it is.
The two 16-bit
variants are NOT compiled: AMD's C++ at this tag includes their headers and references nothing in them (each of the 18
per-pass "...PermutationBlobByIndex" functions takes the 16-bit flag as an unnamed parameter), so those 36 headers are
written EMPTY. No 16-bit type and no minimum precision reaches a shader of ours, and the script refuses a container that
declares either.

WHAT IT CHECKS ON EVERY CONTAINER before a header is written: the checksum Direct3D 12 tests is valid (dxil.dll has
validated and signed it); the shader model is the variant's; the registers in dxc's listing are the ones in the
container's own PSV0 part; and every resource name is one AMD's C++ will find in its own name tables. (AMD's C++
looks each name up there and, on a name it does not know, stops patching that pipeline's bindings and reports nothing:
the review's R2.1. This file said "it fails the pipeline"; it does not. So the check here is the only one there is.)

THE OUTPUT DOES NOT DEPEND ON WHERE IT IS MADE: dxc's container holds no path (measured 2026-10-06: two folders, two
output folders, LF and CRLF sources give the same bytes), the debug files dxc writes beside it are thrown away, and the
headers hold no date and no path. The shaders are compiled from a copy of the kit in a temp folder with a short name,
new on every run, so every run shows it again.
"""
import ctypes, hashlib, os, re, shlex, shutil, struct, subprocess, sys, tempfile, time
from concurrent.futures import ThreadPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.path.insert(0, HERE)
import dxbc

KIT = os.path.join(ROOT, "external", "ffx", "ffx_api_2.3.0")
HEADERS = os.path.join(ROOT, "ffx-own", "shaders")
PIN = {
    "version":        "1.8.2502.11",
    "dxc.exe":        "7c6918a0e2d4e437629fa8549f5ce800970494780f363bbbe1e3d3034f435aee",
    "dxcompiler.dll": "78b5e21a91707b4a567ef281a519c52a45b709561202f54c5ad79c5a315847ed",
    "dxil.dll":       "77e039c905030a641e53658a008b74e90635a5ea9b6b79eabd0f2003bdfca59a",
}
NO_WINDOW = 0x08000000

# ---- AMD's arguments, in the order of its two scripts (BuildFrameInterpolationShaders.bat, BuildOpticalFlowShaders.bat) ----
BASE = ["-DFFX_GPU=1", "-DFFX_IMPLICIT_SHADER_REGISTER_BINDING_HLSL=0"]
API_BASE = ["-E", "CS", "-Wno-for-redefinition", "-Wno-ambig-lit-shift", "-DFFX_HLSL=1"]
VARIANTS = {"": ["-DFFX_HALF=0", "-DFFX_HLSL_SM=62", "-T", "cs_6_2"],
            "_wave64": ["-DFFX_HALF=0", "-DFFX_PREFER_WAVE64=[WaveSize(64)]", "-DFFX_HLSL_SM=66", "-T", "cs_6_6"]}
MODEL = {"": ("cs", 6, 2), "_wave64": ("cs", 6, 6)}
EMPTY = ("_16bit", "_wave64_16bit")
FI = dict(
    key="frameinterpolation", bat="BuildFrameInterpolationShaders.bat", cpp="ffx_frameinterpolation.cpp",
    passes=["ffx_frameinterpolation_reconstruct_and_dilate_pass", "ffx_frameinterpolation_disocclusion_mask_pass",
            "ffx_frameinterpolation_reconstruct_previous_depth_pass", "ffx_frameinterpolation_setup_pass",
            "ffx_frameinterpolation_game_motion_vector_field_pass", "ffx_frameinterpolation_optical_flow_vector_field_pass",
            "ffx_frameinterpolation_pass", "ffx_frameinterpolation_compute_game_vector_field_inpainting_pyramid_pass",
            "ffx_frameinterpolation_compute_inpainting_pyramid_pass", "ffx_frameinterpolation_inpainting_pass",
            "ffx_frameinterpolation_debug_view_pass"],
    args=["-DFFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_SAMPLERS_USE_DATA_HALF=0", "-DFFX_FRAMEINTERPOLATION_OPTION_ACCUMULATE_SAMPLERS_USE_DATA_HALF=0",
          "-DFFX_FRAMEINTERPOLATION_OPTION_REPROJECT_SAMPLERS_USE_DATA_HALF=1", "-DFFX_FRAMEINTERPOLATION_OPTION_POSTPROCESSLOCKSTATUS_SAMPLERS_USE_DATA_HALF=0",
          "-DFFX_FRAMEINTERPOLATION_OPTION_UPSAMPLE_USE_LANCZOS_TYPE=2"] + BASE + API_BASE + ["-DFFX_FRAMEINTERPOLATION_EMBED_ROOTSIG=0"],
    options=["FFX_FRAMEINTERPOLATION_OPTION_LOW_RES_MOTION_VECTORS", "FFX_FRAMEINTERPOLATION_OPTION_JITTER_MOTION_VECTORS", "FFX_FRAMEINTERPOLATION_OPTION_INVERTED_DEPTH"])
OF = dict(
    key="opticalflow", bat="BuildOpticalFlowShaders.bat", cpp="ffx_opticalflow.cpp",
    passes=["ffx_opticalflow_compute_luminance_pyramid_pass", "ffx_opticalflow_compute_optical_flow_advanced_pass_v5",
            "ffx_opticalflow_compute_scd_divergence_pass", "ffx_opticalflow_filter_optical_flow_pass_v5",
            "ffx_opticalflow_generate_scd_histogram_pass", "ffx_opticalflow_prepare_luma_pass",
            "ffx_opticalflow_scale_optical_flow_advanced_pass_v5"],
    args=BASE + API_BASE + ["-DFFX_OPTICALFLOW_EMBED_ROOTSIG=0"],
    options=["FFX_OPTICALFLOW_OPTION_HDR_COLOR_INPUT"])
GROUPS = (FI, OF)
# what FidelityFX_SC.exe takes for itself and dxc never sees (its source at tag v1.1.4, the last one published, reads them)
SC_ONLY = ("-reflection", "-deps=", "-embed-arguments", "-name=", "-output=")


class Refused(Exception):
    pass


def amd_args(group, variant, key):
    """dxc's arguments for one shader, AMD's own and in AMD's order; key is None for the scripts' own {0,1} form"""
    perm = ["-D%s=%s" % (o, "{0,1}" if key is None else (key >> b) & 1) for b, o in enumerate(group["options"])]
    return ["-Zs"] + group["args"] + perm + VARIANTS[variant]


# WHAT OUR COMMAND LINE HOLDS THAT AMD'S SCRIPTS DO NOT, and why. Nothing else may be on it.
OURS_FLAGS = ("-nologo",           # no banner
              "-Vi")               # dxc names every file it opens: the proof holds that list to the manifest
OURS_WITH_VALUE = ("-I",           # the two include folders (AMD's scripts name them too, by paths of their own)
                   "-Fd",          # the debug file dxc writes because of AMD's -Zs, into a folder that is thrown away
                   "-Fo",          # the container
                   "-Fc")          # dxc's listing, for the registers


def command(dxc, kit, folder, group, name, variant, key):
    """THE WHOLE COMMAND LINE of one dxc run: the program, ours, AMD's, the source. compile_one runs exactly this list
    and check_against_bat reads exactly this list, so a token added here is seen there."""
    inc = ["-I", os.path.join(kit, "api", "internal", "gpu"), "-I", os.path.join(kit, "framegeneration", "fsr3", "include", "gpu")]
    src = os.path.join(kit, "framegeneration", "fsr3", "internal", "shaders", name + ".hlsl")
    return [dxc, "-nologo"] + amd_args(group, variant, key) + inc + ["-Vi", "-Fd", folder + "\\", "-Fo", os.path.join(folder, "s.dxil"), "-Fc", os.path.join(folder, "s.txt"), src]


def amd_part(cmd):
    """a whole command line less the program, the source and what the two lists above call ours"""
    out = []; i = 1
    while i < len(cmd) - 1:
        if cmd[i] in OURS_WITH_VALUE: i += 2; continue
        if cmd[i] not in OURS_FLAGS: out.append(cmd[i])
        i += 1
    return out


# ---- the compiler -----------------------------------------------------------------------------------------------------
def file_version(path):
    """a.b.c.d from the file's version resource, read as bytes: the file is not started"""
    d = open(path, "rb").read()
    i = d.find(b"\xbd\x04\xef\xfe")
    if i < 0: return "?"
    ms, ls = struct.unpack_from("<II", d, i + 8)
    return "%d.%d.%d.%d" % (ms >> 16, ms & 0xFFFF, ls >> 16, ls & 0xFFFF)


def dxc_refusal(exe):
    """"" when this dxc.exe and the two libraries beside it are the pinned ones; else the one line that says why not"""
    folder = os.path.dirname(exe)
    for name in ("dxc.exe", "dxcompiler.dll", "dxil.dll"):
        p = exe if name == "dxc.exe" else os.path.join(folder, name)
        if not os.path.isfile(p): return "REFUSED: %s is not there. This build takes dxc %s only (tools\\ffx-own\\gen_shaders.py names it by hash)" % (p, PIN["version"])
        got = hashlib.sha256(open(p, "rb").read()).hexdigest()
        if got != PIN[name]:
            return ("REFUSED: %s is not the %s of dxc %s (it is version %s, SHA-256 %s..., the pin says %s...). Another dxc writes other shader bytes, "
                    "and the committed headers would not verify" % (p, name, PIN["version"], file_version(p), got[:16], PIN[name][:16]))
    return ""


def find_dxc():
    """the pinned dxc.exe on this PC, or Refused with one line"""
    env = os.environ.get("DXC_EXE")
    if env:
        why = dxc_refusal(env)
        if why: raise Refused(why)
        return env
    kits = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), "Windows Kits", "10", "bin")
    found = []
    if os.path.isdir(kits):
        for v in sorted(os.listdir(kits), reverse=True):
            p = os.path.join(kits, v, "x64", "dxc.exe")
            if os.path.isfile(p): found.append(p)
    first = ""
    for p in found:
        why = dxc_refusal(p)
        if not why: return p
        first = first or why
    raise Refused(first or "REFUSED: no dxc.exe under %s. This build takes dxc %s of the Windows SDK only; set DXC_EXE to its full path if it is elsewhere" % (kits, PIN["version"]))


# ---- AMD's scripts against our arguments --------------------------------------------------------------------------------
def bat_lines(kit, group):
    """the four FidelityFX_SC command lines of one of AMD's scripts, variables expanded, as token lists"""
    p = os.path.join(kit, "framegeneration", "fsr3", "dx12", group["bat"])
    var = {}; out = []
    for line in open(p, encoding="utf-8", errors="replace").read().splitlines():
        line = line.strip()
        m = re.match(r"^set (\w+)=(.*)$", line)
        if m:
            # the script's three parameters (%1 %2 %3, then their quotes removed) stay unknown: they print as <name>
            if not re.fullmatch(r"%\d", m.group(2)) and ':"=%' not in m.group(2): var[m.group(1).upper()] = m.group(2)
            continue
        if line.startswith("%FFX_SC%"):
            prev = None
            while prev != line:
                prev = line
                line = re.sub(r"%(\w+)%", lambda m: var.get(m.group(1).upper(), "<%s>" % m.group(1)), line)
            out.append(shlex.split(line.replace("\\", "/")))
    return out


def check_against_bat(kit):
    """our whole command line, less what is ours by name, is AMD's: token for token and in order, the options of AMD's
    own tool, its paths and its name aside"""
    bad = []
    for group in GROUPS:
        lines = bat_lines(kit, group)
        if len(lines) != 4: bad.append("%s has %d FidelityFX_SC lines, not the four variants" % (group["bat"], len(lines))); continue
        for variant, toks in zip(("", "_wave64"), lines[:2]):
            want = []; i = 1
            while i < len(toks) - 1:
                t = toks[i]
                if t == "-I": i += 2; continue
                if not t.startswith(SC_ONLY): want.append(t)
                i += 1
            if "-name=<filename>%s" % variant not in toks: bad.append("%s: line for %r is not named so" % (group["bat"], variant or "base"))
            ours = amd_part(command("dxc", kit, "folder", group, "<filename>", variant, None))
            if want != ours:
                bad.append("%s, variant %r: AMD's arguments are %s and ours are %s" % (group["bat"], variant or "base", " ".join(want), " ".join(ours)))
    return bad


# ---- one compile --------------------------------------------------------------------------------------------------------
def compile_one(job):
    dxc, kit, work, group, name, variant, key = job
    tag = "%s%s_%d" % (name, variant, key)
    folder = os.path.join(work, tag); os.makedirs(folder, exist_ok=True)      # a folder a job: equal shaders give equal debug file names
    args = command(dxc, kit, folder, group, name, variant, key)
    fo, fc, src = args[args.index("-Fo") + 1], args[args.index("-Fc") + 1], args[-1]
    r = subprocess.run(args, capture_output=True, text=True, creationflags=NO_WINDOW)
    opened = [src]; notes = []
    for l in (r.stdout + "\n" + r.stderr).splitlines():
        m = re.match(r"^;?\s*Opening file \[(.*?)\], stack top \[\d+\]", l.strip())
        if m: opened.append(m.group(1))
        elif l.strip() and l.strip() != ";": notes.append(l.strip())
    ok = r.returncode == 0 and os.path.isfile(fo) and os.path.isfile(fc)
    blob = open(fo, "rb").read() if ok else b""
    listing = open(fc, encoding="utf-8", errors="replace").read() if ok else ""
    return (name, variant, key), ok, " | ".join(notes)[:1500], blob, listing, opened


def bindings(listing):
    """the 'Resource Bindings' table of dxc's listing -> {class: [(name, register, count, space)]}"""
    out = {k: [] for k in ("cb", "srvTex", "uavTex", "srvBuf", "uavBuf", "sampler", "rtas")}
    lines = listing.splitlines()
    try: i = next(k for k, l in enumerate(lines) if l.startswith("; Resource Bindings:"))
    except StopIteration: return out
    for l in lines[i + 4:]:
        if not l.startswith("; ") or len(l.split()) < 8: break
        _, name, typ, fmt, dim, rid, bind, count = l.split()[:8]
        m = re.match(r"^(cb|t|u|s)(\d+)(?:,space(\d+))?$", bind)
        if not m: raise Refused("cannot read the register %r in dxc's listing: %s" % (bind, l))
        slot, space = int(m.group(2)), int(m.group(3) or 0)
        if count == "unbounded": raise Refused("an unbounded array in dxc's listing, which AMD's backend has no case for here: %s" % l)
        buf = fmt in ("struct", "byte")
        if typ == "cbuffer": k = "cb"
        elif typ == "sampler": k = "sampler"
        elif typ == "texture": k = "rtas" if dim == "ras" else ("srvBuf" if buf else "srvTex")
        elif typ == "UAV": k = "uavBuf" if buf else "uavTex"
        else: raise Refused("unknown resource type %r in dxc's listing: %s" % (typ, l))
        out[k].append((name, slot, int(count), space))
    return out


def cpp_names(kit, group):
    """{'srv': names, 'uav': names, 'cb': names} from the name tables of AMD's C++, the ones patchResourceBindings searches"""
    text = open(os.path.join(kit, "framegeneration", "fsr3", "internal", group["cpp"]), encoding="utf-8", errors="replace").read()
    out = {}
    for m in re.finditer(r"static const (?:ResourceBinding|Binding) (srv|uav|cb)\w*\[\]\s*=\s*\{(.*?)\n\};", text, re.S):
        out[m.group(1)] = set(re.findall(r'L"([^"]+)"', m.group(2)))
    if sorted(out) != ["cb", "srv", "uav"]: raise Refused("%s does not hold the three name tables this check reads (found %s)" % (group["cpp"], sorted(out)))
    return out


def check_container(group, name, variant, key, blob, bnd, names):
    """the reasons this container may not go into a header; [] when there is none"""
    who = "%s%s key %d" % (name, variant, key); bad = []
    if not dxbc.signed(blob): bad.append("%s: the container's checksum is %s: dxil.dll did not sign it" % (who, "zero" if blob[4:20] == b"\0" * 16 else "wrong"))
    if dxbc.model(blob) != MODEL[variant]: bad.append("%s: shader model %s_%d_%d, the variant is %s_%d_%d" % ((who,) + dxbc.model(blob) + MODEL[variant]))
    if dxbc.features(blob) & dxbc.SIXTEEN_BIT: bad.append("%s declares %s" % (who, ", ".join(dxbc.feature_names(dxbc.features(blob) & dxbc.SIXTEEN_BIT))))
    lanes = dxbc.wave_lanes(blob)
    if lanes not in (dxbc.ANY_LANES, dxbc.WAVE64) or (lanes == dxbc.WAVE64 and not variant):
        bad.append("%s: its entry expects %s wave lanes: a base shader has no [WaveSize], a wave64 one has none or [WaveSize(64)]" % (who, lanes))
    h = dxbc.shader_hash(blob)
    if not h or dxbc.debug_name(blob) != h + ".pdb": bad.append("%s: no shader hash, or a debug name that is not the hash (%r, %r)" % (who, h, dxbc.debug_name(blob)))
    listed = sorted((k, e[3], e[1], e[1] + e[2] - 1) for k in bnd for e in bnd[k])
    psv = dxbc.psv_resources(blob)
    if psv is None or sorted(psv) != listed: bad.append("%s: dxc's listing binds %s and the container's PSV0 part says %s" % (who, listed, psv))
    # a sampler has no name table: AMD's backend makes its static samplers from the pass's own description, not from the header
    for k, table in (("cb", "cb"), ("srvTex", "srv"), ("srvBuf", "srv"), ("uavTex", "uav"), ("uavBuf", "uav"), ("sampler", None)):
        for e in bnd[k]:
            if table and e[0] not in names[table]: bad.append("%s binds %r, which is not in the %s name table of %s" % (who, e[0], table, group["cpp"]))
            if e[2] != 1 or e[3] != 0: bad.append("%s binds %r with count %d in space %d: only count 1 in space 0 has been read through AMD's backend" % (who, e[0], e[2], e[3]))
    if bnd["rtas"]: bad.append("%s binds an acceleration structure, which no pass of this tag does" % who)
    return bad


# ---- the headers --------------------------------------------------------------------------------------------------------
CLASSES = [("cb", "CBV", "numConstantBuffers", "constantBuffer"), ("srvTex", "TextureSRV", "numSRVTextures", "srvTexture"),
           ("uavTex", "TextureUAV", "numUAVTextures", "uavTexture"), ("srvBuf", "BufferSRV", "numSRVBuffers", "srvBuffer"),
           ("uavBuf", "BufferUAV", "numUAVBuffers", "uavBuffer"), ("sampler", "Sampler", "numSamplers", "sampler"),
           ("rtas", "RTAccelerationStructure", "numRTAccelerationStructures", "rtAccelerationStructure")]
NOTE = ("// Made by tools\\ffx-own\\gen_shaders.py (the mod's stand-in for AMD's unpublished FidelityFX_SC.exe) with dxc %s.\n"
        "// Do not edit: gen_shaders.py --verify holds this file to a regeneration, byte for byte.\n" % PIN["version"])


def header_text(group, name, variant, results):
    """the text of <name><variant>_permutations.h; results is {key: (container, bindings)}"""
    full = name + variant
    order = []; uniq = {}; table = []
    for key in range(1 << len(group["options"])):
        blob, bnd = results[key]
        h = dxbc.shader_hash(blob)
        if h not in uniq: uniq[h] = (blob, bnd); order.append(h)
        elif uniq[h][0] != blob: raise Refused("%s: two different containers carry the shader hash %s" % (full, h))
        table.append(order.index(h))
    sized = set(dxbc.wave_lanes(uniq[h][0]) == dxbc.WAVE64 for h in order)
    if len(sized) != 1: raise Refused("%s: some of its containers carry [WaveSize(64)] and some do not" % full)
    how = "cs_6_2" if not variant else ("cs_6_6 by AMD's wave64 arguments; [WaveSize(64)] is on the entry" if True in sized else
                                        "cs_6_6 by AMD's wave64 arguments; no [WaveSize] on the entry (this pass's source does not use AMD's FFX_PREFER_WAVE64)")
    w = ["// %s_permutations.h: AMD's %s.hlsl (FidelityFX SDK v2.3.0, MIT), compiled: %d container(s) for %d permutation(s), %s." % (
        full, name, len(order), len(table), how), NOTE.rstrip("\n"), "#pragma once", "#include <stdint.h>", ""]
    for h in order:
        blob, bnd = uniq[h]
        p = "g_%s_%s" % (full, h)
        for k, cname, _, _ in CLASSES:
            if not bnd[k]: continue
            w.append("static const char* %s_%sResourceNames[] = { %s };" % (p, cname, ", ".join('"%s"' % e[0] for e in bnd[k])))
            w.append("static const uint32_t %s_%sResourceBindings[] = { %s };" % (p, cname, ", ".join(str(e[1]) for e in bnd[k])))
            w.append("static const uint32_t %s_%sResourceCounts[] = { %s };" % (p, cname, ", ".join(str(e[2]) for e in bnd[k])))
            w.append("static const uint32_t %s_%sResourceSpaces[] = { %s };" % (p, cname, ", ".join(str(e[3]) for e in bnd[k])))
        w.append("static const uint32_t %s_size = %d;" % (p, len(blob)))
        w.append("static const unsigned char %s_data[] = {" % p)
        for o in range(0, len(blob), 32): w.append(",".join(str(b) for b in blob[o:o + 32]) + ("," if o + 32 < len(blob) else ""))
        w.append("};"); w.append("")
    w.append("typedef union %s_PermutationKey {" % full); w.append("    struct {")
    for o in group["options"]: w.append("        uint32_t %s : 1;" % o)
    w += ["    };", "    uint32_t index;", "} %s_PermutationKey;" % full, ""]
    w += ["typedef struct %s_PermutationInfo {" % full, "    const uint32_t       blobSize;", "    const unsigned char* blobData;", "    const char*          entryName;"]
    for k, cname, num, fld in CLASSES:
        w += ["    const uint32_t  %s;" % num, "    const char**    %sNames;" % fld, "    const uint32_t* %sBindings;" % fld, "    const uint32_t* %sCounts;" % fld, "    const uint32_t* %sSpaces;" % fld]
    w += ["} %s_PermutationInfo;" % full, "", "static const uint32_t g_%s_IndirectionTable[] = { %s };" % (full, ", ".join(str(t) for t in table)), "",
          "static const %s_PermutationInfo g_%s_PermutationInfo[] = {" % (full, full)]
    for h in order:
        blob, bnd = uniq[h]
        p = "g_%s_%s" % (full, h)
        row = ["%s_size" % p, "%s_data" % p, '"CS"']
        for k, cname, _, _ in CLASSES:
            row += [str(len(bnd[k])), "%s_%sResourceNames" % (p, cname), "%s_%sResourceBindings" % (p, cname), "%s_%sResourceCounts" % (p, cname), "%s_%sResourceSpaces" % (p, cname)] if bnd[k] else ["0"] * 5
        w.append("    { %s }," % ", ".join(row))
    w += ["};", ""]
    return "\n".join(w), [uniq[h][0] for h in order]


def empty_text(name, variant):
    return ("// %s%s_permutations.h: EMPTY ON PURPOSE. AMD's C++ at tag v2.3.0 includes this header and references nothing in it:\n"
            "// the 16-bit variants are not compiled for the mod's own build (tools\\ffx-own\\gen_shaders.py says why).\n%s#pragma once\n" % (name, variant, NOTE))


def generate(kit=KIT, threads=4, only=None, dxc=None):
    """-> ({file name: text}, facts). Compiles in a temp folder and removes it. Raises Refused with the first reason."""
    ctypes.windll.kernel32.SetPriorityClass(ctypes.windll.kernel32.GetCurrentProcess(), 0x00004000)   # BELOW_NORMAL: dxc inherits it
    dxc = dxc or find_dxc()
    work = tempfile.mkdtemp(prefix="ffxs-")
    try:
        # the short copy: AMD's files include each other through chains of "../" that are joined unresolved, and a kit deep
        # in a long path passes the 260 characters Windows allows (build-ffx-own.ps1 met it and says more)
        kit = shutil.copytree(os.path.abspath(kit), os.path.join(work, "k"))
        bad = check_against_bat(kit)
        if bad: raise Refused("our dxc arguments are not the ones in AMD's scripts: " + "; ".join(bad))
        jobs =[(dxc, kit, work, g, n, v, key) for g in GROUPS for n in g["passes"] if not only or n in only for v in VARIANTS for key in range(1 << len(g["options"]))]
        t0 = time.perf_counter(); res = {}; opened = set(); fails = []
        with ThreadPoolExecutor(threads) as ex:
            for (name, variant, key), ok, notes, blob, listing, op in ex.map(compile_one, jobs):
                if not ok: fails.append("%s%s key %d: %s" % (name, variant, key, notes)); continue
                if notes: fails.append("%s%s key %d: dxc said: %s" % (name, variant, key, notes))
                m = re.search(r"^; WaveSize=(\d+)", listing, re.M)      # the listing's line for a shader with the attribute; the two counts for one without
                said = (int(m.group(1)),) * 2 if m else tuple(int(x) for x in re.findall(r"^; M(?:in|ax)imumExpectedWaveLaneCount: (\d+)", listing, re.M))
                if said != dxbc.wave_lanes(blob): fails.append("%s%s key %d: dxc's listing says the entry expects %s wave lanes and the container's PSV0 part says %s" % (name, variant, key, said, dxbc.wave_lanes(blob)))
                res.setdefault((name, variant), {})[key] = (blob, bindings(listing))
                for p in op: opened.add(os.path.normpath(p))
        if fails: raise Refused("dxc failed or warned on %d of %d shaders; the first: %s" % (len(fails), len(jobs), fails[0]))
        files = {}; containers = []; feats = 0; problems = []
        for g in GROUPS:
            names = cpp_names(kit, g)
            for n in g["passes"]:
                if only and n not in only: continue
                for v in VARIANTS:
                    for key, (blob, bnd) in sorted(res[(n, v)].items()):
                        problems += check_container(g, n, v, key, blob, bnd, names); feats |= dxbc.features(blob)
                    text, blobs = header_text(g, n, v, res[(n, v)])
                    files[n + v + "_permutations.h"] = text; containers += [(v, b) for b in blobs]
                for v in EMPTY: files[n + v + "_permutations.h"] = empty_text(n, v)
        if problems: raise Refused("%d container(s) may not go into a header; the first: %s" % (len(problems), problems[0]))
        reads = {}                # every file of the kit dxc opened -> git's id of the bytes it opened, hashed here, in the copy it compiled from
        for p in opened:
            rel = os.path.relpath(p, kit)
            if rel.startswith(".."): raise Refused("dxc opened a file outside the kit: %s" % p)
            data = open(p, "rb").read()
            reads[rel.replace(os.sep, "/")] = hashlib.sha1(b"blob %d\0" % len(data) + data).hexdigest()
        facts = dict(dxc=dxc, runs=len(jobs), seconds=time.perf_counter() - t0, base=sum(1 for v, b in containers if not v), wave64=sum(1 for v, b in containers if v),
                     wavesize=sum(1 for v, b in containers if dxbc.wave_lanes(b) == dxbc.WAVE64),
                     container_bytes=sum(len(b) for v, b in containers), features=dxbc.feature_names(feats), reads=sorted(reads), read_ids=reads,
                     header_bytes=sum(len(t.encode("ascii")) for t in files.values()))
        return files, facts
    finally:
        shutil.rmtree(work, ignore_errors=True)


def write(files, folder):
    os.makedirs(folder, exist_ok=True)
    for f in os.listdir(folder):
        if f.endswith("_permutations.h") and f not in files: os.remove(os.path.join(folder, f))
    for f, text in files.items():
        open(os.path.join(folder, f), "wb").write(text.encode("ascii"))


def compare(files, folder):
    """the differences between a regeneration and the headers in a folder; [] when there is none"""
    bad = []
    have = set(f for f in os.listdir(folder)) if os.path.isdir(folder) else set()
    for f in sorted(files):
        p = os.path.join(folder, f)
        if f not in have: bad.append("%s is not in %s" % (f, folder)); continue
        d = open(p, "rb").read(); want = files[f].encode("ascii")
        if d != want:
            i = next((k for k in range(min(len(d), len(want))) if d[k] != want[k]), min(len(d), len(want)))
            bad.append("%s is not what the shader compiles to: %d bytes against %d, the first difference at byte %d (line %d)%s" % (
                f, len(d), len(want), i, d[:i].count(b"\n") + 1, "; it has CRLF line endings" if d.replace(b"\r\n", b"\n") == want else ""))
    for f in sorted(have - set(files)): bad.append("%s is in %s and is not a header this script makes" % (f, folder))
    return bad


def summary(facts, files):
    return ("%d dxc runs in %.1f s with dxc %s; %d containers (%d cs_6_2, %d cs_6_6 by AMD's wave64 arguments, %d of those with [WaveSize(64)] on the entry), %d bytes, "
            "all signed by dxil.dll; they declare: %s; "
            "%d headers (%d empty 16-bit ones), %d bytes" % (facts["runs"], facts["seconds"], PIN["version"], facts["base"] + facts["wave64"], facts["base"], facts["wave64"], facts["wavesize"],
                                                              facts["container_bytes"], ", ".join(facts["features"]) or "nothing", len(files),
                                                              sum(1 for f in files if "_16bit_" in f), facts["header_bytes"]))


def main():
    a = sys.argv[1:]
    kit = KIT; out = HEADERS; verify = False; threads = 4; reads = None; against = None
    while a:
        o = a.pop(0)
        if o == "--verify": verify = True
        elif o == "--kit" and a: kit = a.pop(0)
        elif o == "--out" and a: out = a.pop(0)
        elif o == "--against" and a: against = a.pop(0)
        elif o == "--reads" and a: reads = a.pop(0)
        elif o == "--threads" and a: threads = int(a.pop(0))
        else: raise SystemExit("usage: gen_shaders.py [--verify [--against <folder>]] [--kit <folder>] [--out <folder>] [--reads <file>] [--threads <n>]")
    try:
        files, facts = generate(kit, threads)
    except Refused as e:
        print(str(e)); print("FAIL"); return 1
    print(summary(facts, files))
    if reads: open(reads, "w", newline="\n").write("".join("%s %s\n" % (facts["read_ids"][p], p) for p in facts["reads"]))
    if not verify:
        write(files, out)
        print("wrote %d headers into %s" % (len(files), out)); return 0
    bad = compare(files, against or HEADERS)
    for w in bad[:20]: print("  " + w)
    print("the %d committed headers against a regeneration: %s" % (len(files), "byte-equal, every one" if not bad else "%d DIFFER" % len(bad)))
    print("PASS" if not bad else "FAIL")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
