#!/usr/bin/env python3
"""The device thread is told every render state this mod may set, and this proves it.

    python tools/check-body-touch.py

WHY. With inject_worker on, the injection body runs on the device thread's worker. It sets
render states of its own and puts them back, so the stream is unchanged by it, but WHILE IT
RUNS the device holds ours. The layer's shadow may fetch an unknown render state from DXVK
directly; beside a running body that is only exact for a state the body does not set. So
devthread/dt_trk.h declares the set (kBodyRs), the post makes those states known before the
body counts as out, and every OTHER render state may be fetched beside it without waiting
(the owner's log of 2026-09-19: the game asks for D3DRS_LIGHTING once a frame right behind
the seam, and waiting for the body before answering cost 0.18 to 0.45 ms a frame).

THE RULE THIS ENFORCES is a superset, because a superset is free and a miss is a wrong
answer handed to the game: EVERY render state the mod sets ANYWHERE, body or not, must be in
kBodyRs, and none may be set through a variable, which this could not see.

THE CONTROL: the script must find the sites it is known to have (IvtSetupPass's ten states
and VelDrain's D3DRS_ZFUNC), so a regex that silently matched nothing fails instead of passing.
Comments and string literals are stripped first. Exit 0 means the declaration is complete.

WHAT THIS DOES NOT PROVE (C1's body audit R8, 2026-09-29): that the body puts back what it sets, or that the sampler mask
(dtBodySampFree) is right; the sampler indices are variables at most sites. The audit's reading is that evidence, and
development builds measure both masks in the game: every Set a body makes on the worker is checked against them
(dt_core.h, DtBodyDecl; the log's "the injection body's declarations" line; the harness's NC34 plants a wrong mask).
"""
import io, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(ROOT, "source")
DECL_FILE = os.path.join(SRC, "devthread", "dt_trk.h")
MUST_FIND = {"D3DRS_ZENABLE", "D3DRS_ZWRITEENABLE", "D3DRS_ALPHABLENDENABLE", "D3DRS_ALPHATESTENABLE", "D3DRS_CULLMODE",
             "D3DRS_COLORWRITEENABLE", "D3DRS_SCISSORTESTENABLE", "D3DRS_STENCILENABLE", "D3DRS_FOGENABLE",
             "D3DRS_SRGBWRITEENABLE", "D3DRS_ZFUNC"}


def strip(s):
    out, i, n = [], 0, len(s)
    while i < n:
        c = s[i]
        if c == '"' or c == "'":
            j = i + 1
            while j < n and s[j] != c:
                j += 2 if s[j] == "\\" else 1
            out.append(c + " " * (j - i - 1) + c); i = j + 1
        elif s.startswith("//", i):
            j = s.find("\n", i); j = n if j < 0 else j
            out.append(" " * (j - i)); i = j
        elif s.startswith("/*", i):
            j = s.find("*/", i); j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", s[i:j])); i = j
        else:
            out.append(c); i += 1
    return "".join(out)


def main():
    fails = []
    decl_src = strip(io.open(DECL_FILE, encoding="utf-8-sig").read())
    m = re.search(r"kBodyRs\[\]\s*=\s*\{([^}]*)\}", decl_src)
    if not m:
        print("FAIL: kBodyRs[] not found in devthread/dt_trk.h"); return 1
    declared = set(re.findall(r"D3DRS_[A-Z0-9_]+", m.group(1)))
    print("declared in dt_trk.h: %d render states" % len(declared))

    found = {}
    for dirpath, _, names in os.walk(SRC):
        for name in names:
            if not name.endswith((".h", ".cpp")):
                continue
            path = os.path.join(dirpath, name)
            rel = os.path.relpath(path, ROOT).replace("\\", "/")
            if "/devthread/" in rel and name != "dt_trk.h":
                continue      # the layer's own wrappers forward the GAME's states; they set none of their own
            code = strip(io.open(path, encoding="utf-8-sig").read())
            for mm in re.finditer(r"\b(?:orig)?SetRenderState\s*\(", code):
                line_start = code.rfind("\n", 0, mm.start()) + 1
                head = code[line_start:mm.start()]
                if re.search(r"\b(static|typedef|HRESULT)\b", head) or "hkSetRenderState" in code[mm.start() - 2:mm.end()]:
                    continue  # a declaration, a typedef, the hook itself
                i, depth = mm.end(), 1
                while i < len(code) and depth:
                    depth += code[i] == "("; depth -= code[i] == ")"; i += 1
                args = code[mm.end():i - 1]
                ln = code.count("\n", 0, mm.start()) + 1
                parts = [a.strip() for a in args.split(",")]
                state = parts[1] if parts and parts[0] in ("dev", "d", "ivtDev", "device") and len(parts) > 2 else parts[0]
                lit = re.fullmatch(r"D3DRS_[A-Z0-9_]+", state)
                if not lit:
                    # the hook's own forward of the game's call names a variable, legitimately
                    if re.search(r"\b(state|State|s)\b", state) and ("hkSetRenderState" in code[max(0, line_start - 4000):mm.start()]):
                        continue
                    fails.append("%s:%d sets a render state through an expression (%s): it cannot be declared" % (rel, ln, state[:40]))
                    continue
                found.setdefault(state, []).append("%s:%d" % (rel, ln))

    print("set somewhere in source/: %d distinct render states at %d sites" % (len(found), sum(len(v) for v in found.values())))
    for s in sorted(MUST_FIND - set(found)):
        fails.append("CONTROL: %s is known to be set by the mod and this script did not find it" % s)
    for s in sorted(set(found) - declared):
        fails.append("%s is set at %s and is NOT in kBodyRs (devthread/dt_trk.h)" % (s, ", ".join(found[s][:3])))
    for s in sorted(declared - set(found)):
        print("  note: %s is declared and no longer set anywhere (a superset is harmless)" % s)
    for f in fails:
        print("FAIL:", f)
    print("PASS" if not fails else "FAILED")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
