#!/usr/bin/env python3
"""Every call the mod makes to DXVK outside D3D9 is inside a DT_DIRECT() scope.

    python tools/check-dt.py

With threaded=1 the mod's D3D9 calls are recorded and reach DXVK a moment later
on another thread. Anything that talks to DXVK or Vulkan DIRECTLY (the interop
device, an interop texture's image and layout, the extended swapchain, a submit
on DXVK's queue) must first drain the ring, or it sees a DXVK that has not yet
received the D3D9 calls made before it. DT_DIRECT() does exactly that.

This finds every such call site in the sources, finds the function it is in,
and checks that the function opens a DT_DIRECT() scope BEFORE the call: either
itself, or because every caller of it does. Comments and string literals are
stripped first, so a comment that mentions an interop call proves nothing and
cannot fail anything. Exit 0 means every site is covered.

THE CONTROL: the script also checks that it FINDS the call sites it must find
(IvtInject's FlushRenderingCommands, IvtHome's, HdrColorSpace's SetColorSpace),
so a regex that silently matched nothing would fail rather than pass.
"""
import io, os, re, sys
import fg_machine_text   # (fg_machine.h is in parts included in place: it is read with them pasted back, tools/fg_machine_text.py)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# hdr_cga.h JOINED THIS LIST ON 2026-09-18 (the audit found it missing): 500 lines that bind
# render targets, upload constants and issue three draws, none of which this guard was reading,
# so it printed PASS over a file it had never looked at.
FILES = ["source/DLSS-IV.cpp", "source/transport/DlssIvTransport.h", "source/hdr_encode.h",
         "source/hdr.h", "source/hdr_cga.h",
         # THE MACHINE (phase 1C, 2026-09-25, with the frame generation rig until round 8's deep strip took the rig out), so no
         # DXVK or Vulkan call it adds later can land unread (docs/reports/fg-1c/map-final.md, section 3's gate steps).
         "source/fg/fg_machine.h",
         # THE PRODUCT'S OWN VULKAN LINES AND THE BINDING THAT ARMS (phase 3, stage 8, 2026-10-10): fg_vklines.h's one call of
         # DXVK's (FgvBbQuery) and its copies are reached from the product now, through fg_arm.h's seams and fg_arm_life.h
         # (docs/reports/fg-3/vklines-2026-10-10/REPORT.md, section 11: "add the file to tools/check-dt.py's list").
         # fg_product.h with them: the shadow shape's seams are the armed binding's base, and this list had never read it.
         "source/fg/fg_product.h", "source/fg/fg_vklines.h", "source/fg/fg_arm.h", "source/fg/fg_arm_life.h"]

# A direct conversation with DXVK or Vulkan. Setup that happens before any
# device exists (UnlockAdditionalFormats on the IDirect3D9) is listed apart.
# AddRef, Release and QueryInterface on an interop object are reference counting,
# which no queued D3D9 call can be ordered against, so they are not sites: the
# first run of this script flagged hkReset's velIDev->Release() and was right to
# be narrowed, not obeyed.
IUNK = r"(?!AddRef\b|Release\b|QueryInterface\b)"
SITE = re.compile(r"(ivtIDev\s*->\s*" + IUNK + r"\w+|velIDev\s*->\s*" + IUNK + r"\w+|->\s*GetVulkanImageInfo\s*\("
                  r"|xsc\s*->\s*" + IUNK + r"\w+|ivtQueueSubmit\s*\(|FeedVk(?:CopyImage|BlitImage)\s*\()")
BEFORE_DEVICE = {"HdrUnlockFormats"}          # runs in hkCreateDevice, before the device exists
# Not ordered against D3D9: they touch only our own Vulkan objects (our command
# buffers, our timeline semaphore), never a D3D9 resource's image or DXVK's queue.
ORDER_FREE = {"FeedVkTimelineValue"}
# THE DESK TOOL'S THREE SPANS (fg_vklines.h, phase 3 stage 9): whole buffers the desk tool tools/test-fg-vklines records in
# one call each. THE MOD NEVER CALLS THEM: its machine owns the order of a span and calls the copies one by one through
# its binding's seams, which open DT_DIRECT (fg_arm.h). So they are not callers this guard can ask about, and they are
# left out of the walk. THE CONTROL below fails the day any file of the list calls one: it is then a caller like any
# other, and comes off this set.
DESK_ONLY = {"FgvSpanHand", "FgvSpanWrite", "FgvSpanRestore"}

def strip(src):
    out, i, n = [], 0, len(src)
    while i < n:
        c = src[i]
        if src.startswith("//", i):
            j = src.find("\n", i); j = n if j < 0 else j
            out.append(" " * (j - i)); i = j
        elif src.startswith("/*", i):
            j = src.find("*/", i + 2); j = n if j < 0 else j + 2
            out.append(re.sub(r"[^\n]", " ", src[i:j])); i = j
        elif c in "\"'":
            j = i + 1
            while j < n and src[j] != c:
                j += 2 if src[j] == "\\" else 1
            out.append(c + " " * (j - i - 1) + c); i = j + 1
        else:
            out.append(c); i += 1
    return "".join(out)

FUNC = re.compile(r"^(?:static\s+|inline\s+|__forceinline\s+|template\s*<[^>]*>\s*)*[\w:<>\*&\s]+?\b(\w+)\s*\([^;{}]*\)\s*\{", re.M)

def functions(text):
    """(name, body_start, body_end) for every top-level function body."""
    res = []
    for m in FUNC.finditer(text):
        name, start = m.group(1), m.end() - 1
        if name in ("if", "for", "while", "switch", "return", "sizeof"):
            continue
        depth, j = 0, start
        while j < len(text):
            if text[j] == "{": depth += 1
            elif text[j] == "}":
                depth -= 1
                if depth == 0: break
            j += 1
        res.append((name, start, j))
    return res

fail, sites, funcs = [], [], {}
for rel in FILES:
    path = os.path.join(ROOT, rel)
    if not os.path.exists(path):
        continue
    text = strip(fg_machine_text.read(path, encoding="utf-8", errors="replace"))
    fl = functions(text)
    for name, a, b in fl:
        funcs.setdefault(name, []).append((rel, text, a, b))
    for m in SITE.finditer(text):
        owner = [f for f in fl if f[1] < m.start() < f[2]]
        if not owner:
            fail.append("%s: an interop call outside any function: %s" % (rel, m.group(0)))
            continue
        name, a, b = min(owner, key=lambda f: f[2] - f[1])
        line = text.count("\n", 0, m.start()) + 1
        sites.append((rel, line, name, m.group(0).strip(), text, a, m.start()))

def guarded(name, text, a, pos, seen=()):
    """DT_DIRECT() in this function before pos, or in every caller before its call."""
    if name in BEFORE_DEVICE or name in ORDER_FREE:
        return True
    if "DT_DIRECT()" in text[a:pos]:
        return True
    if name in seen:
        return False
    callers = []
    for fname, defs in funcs.items():
        if fname in DESK_ONLY:
            continue
        for rel, t, fa, fb in defs:
            for cm in re.finditer(r"\b%s\s*\(" % re.escape(name), t[fa:fb]):
                callers.append((fname, t, fa, fa + cm.start()))
    return bool(callers) and all(guarded(f, t, fa, p, seen + (name,)) for f, t, fa, p in callers)

for dname in sorted(DESK_ONLY):
    if dname not in funcs:
        fail.append("CONTROL: %s is in DESK_ONLY and no file of the list defines it; the set is stale" % dname)
    for fname, defs in funcs.items():
        for rel, t, fa, fb in defs:
            if re.search(r"\b%s\s*(?:<[^>;(){}]*>)?\s*\(" % re.escape(dname), t[fa:fb]):
                fail.append("%s: %s calls %s, a span this guard leaves out because the mod never calls it: take it out of DESK_ONLY" % (rel, fname, dname))

# IvtInjectBody, not IvtInject, since 2026-09-18: the injection was split at the
# DT_DIRECT boundary so that moving it to the worker is a change of caller and not a
# change of code (docs/DESIGN-INJECTION-ON-THE-WORKER.md). The control caught the move
# on the first build, which is exactly what it is for.
# Flush (stage S4, map section 3's gate step): the product's flush, FgP::Flush in source/fg/fg_product.h, the one
# FlushRenderingCommands its spans make (the frame generation rig's FgRigB::Flush, the first of the name, went with the rig).
must_find = {("IvtInjectBody", "ivtIDev->FlushRenderingCommands"), ("IvtHome", "ivtIDev->FlushRenderingCommands"),
             ("HdrColorSpace", "xsc->SetColorSpace"), ("Flush", "ivtIDev->FlushRenderingCommands")}
found = {(s[2], re.sub(r"\s+", "", s[3])) for s in sites}
for want in must_find:
    if want not in found:
        fail.append("CONTROL: did not find %s in %s; the search is broken, not the code" % (want[1], want[0]))

# THE SLOT IN THE HEADER IS THE CALLEE (stage 2.0, then O9 of the 2026-09-17 sweep). A
# recorded device call is its header and its arguments: the worker calls dtDx[slot] on
# dtDev, so a wrong DT_SLOT is no longer a wrong observer, it is a WRONG METHOD called
# with another method's arguments. The rules, for every file under source/devthread/:
#   1. DtCmdDev appears only in dt_device_rec.h, and every site names DT_SLOT(s).
#   2. That s is the dtDx[] index the same wrapper calls on its direct path: every
#      dtDx[x] between the wrapper's signature and the site must be dtDx[s], and there
#      must be at least one (a wrapper with no direct path would prove nothing).
#   3. DtCmdX, the explicit form, appears only in dt_res.h, names no DT_SLOT, and its op
#      is RELEASE2, XCALL0 or PRESENT: a CALL0 written through DtCmdX would be replayed
#      as a device call on whatever slot its flag bits happened to spell.
#   4. Nothing writes dtDx[] but the vtable hook's install in DLSS-IV.cpp: the worker
#      reads the table on every replayed call with no fence.
# Controls: the recorder has dozens of sites, dt_res.h a few, and the one writer is found.
slot_sites = 0
res_sites = 0
dt_dir = os.path.join(ROOT, "source/devthread")
for fn in sorted(os.listdir(dt_dir)) if os.path.isdir(dt_dir) else []:
    if not fn.endswith(".h"):
        continue
    flines = io.open(os.path.join(dt_dir, fn), "r", encoding="utf-8", errors="replace").read().split("\n")
    for ln, line in enumerate(flines, 1):
        if line.lstrip().startswith("//"):
            continue
        if re.search(r"\bDtCmd\(", line):
            fail.append("%s:%d: DtCmd is gone (O9); a device call is DtCmdDev, anything else DtCmdX: %s" % (fn, ln, line.strip()))
        if "DtCmdDev(" in line and not re.search(r"\bDtCmdDev\(DWORD\b", line):
            if fn != "dt_device_rec.h":
                fail.append("%s:%d: a DtCmdDev outside the recorder: %s" % (fn, ln, line.strip()))
                continue
            slot_sites += 1
            m = re.search(r"DT_SLOT\((\w+)\)", line)
            if not m:
                fail.append("%s:%d: a DtCmdDev with no DT_SLOT(s): the worker would call slot 0: %s" % (fn, ln, line.strip()))
                continue
            # back to the wrapper's own signature (or the macro's), collecting dtDx[x]
            seen = []
            k = ln - 1
            while k >= 1:
                seen += re.findall(r"dtDx\[(\w+)\]", flines[k - 1])
                if re.search(r"__stdcall dt_\w*\(|__stdcall dt_##name\(", flines[k - 1]) and "typedef" not in flines[k - 1]:
                    break
                k -= 1
            if k < 1:
                fail.append("%s:%d: no wrapper signature found above this DtCmdDev" % (fn, ln))
            elif not seen:
                fail.append("%s:%d: DT_SLOT(%s) with no dtDx[] on the wrapper's direct path to check it against" % (fn, ln, m.group(1)))
            elif any(x != m.group(1) for x in seen):
                fail.append("%s:%d: DT_SLOT(%s) in a wrapper whose direct path calls dtDx[%s]" % (fn, ln, m.group(1), ", ".join(sorted(set(seen)))))
        if "DtCmdX(" in line and not re.search(r"\bDtCmdX\(DWORD\b", line):
            if fn != "dt_res.h":
                fail.append("%s:%d: a DtCmdX outside dt_res.h: %s" % (fn, ln, line.strip()))
                continue
            res_sites += 1
            if "DT_SLOT(" in line:
                fail.append("%s:%d: an explicit command carries no slot: %s" % (fn, ln, line.strip()))
            if not re.search(r"DtCmdX\(DT_OP_(RELEASE2|XCALL0|PRESENT|SBAPPLY)\b", line):
                fail.append("%s:%d: an explicit command whose op is not RELEASE2, XCALL0, PRESENT or SBAPPLY (C1's recorded Apply): %s" % (fn, ln, line.strip()))
if slot_sites < 25:
    fail.append("CONTROL: found only %d DtCmdDev sites in dt_device_rec.h; the search is broken, not the code" % slot_sites)
if res_sites < 3:
    fail.append("CONTROL: found only %d DtCmdX sites in dt_res.h" % res_sites)
writers = []
for root_, _, names in os.walk(os.path.join(ROOT, "source")):
    for nm in names:
        if not nm.endswith((".h", ".cpp")):
            continue
        rel_ = os.path.relpath(os.path.join(root_, nm), ROOT).replace("\\", "/")
        for ln, line in enumerate(io.open(os.path.join(root_, nm), "r", encoding="utf-8", errors="replace"), 1):
            if line.lstrip().startswith("//"):
                continue
            if re.search(r"memcpy\(\s*dtDx\b|\bdtDx\[[^\]]*\]\s*=[^=]|memset\(\s*dtDx\b", line):
                writers.append("%s:%d" % (rel_, ln))
if len(writers) != 1 or not writers[0].startswith("source/DLSS-IV.cpp:"):
    fail.append("dtDx[] must have exactly one writer, the vtable hook's install in DLSS-IV.cpp; found: %s" % (", ".join(writers) or "none (CONTROL: the search is broken)"))
print("recorded-call slots: %d recorder sites and %d explicit sites checked; dtDx[] written at %s" % (slot_sites, res_sites, ", ".join(writers)))

# EVERY RAW SUBMIT ON DXVK'S QUEUE IS KNOWN (phase 1C, stage S2, 2026-09-25; the map's G-N1 and its grep proof). Every
# ivtQueueSubmit site outside source/fg is one of the four named here: the frame-end marker's (LatEndSignal), the injection
# body's (the worker whenever the layer is armed), the depth snapshot's and the same-frame home's. A new site fails here until
# it is named. (Until round 8's deep strip the last two were followed by the frame generation rig's count of a submit that
# could land inside a rig pair, FgRigCountRawSubmit; the rig and its count are gone.)
RAW_BY_NAME = {"LatEndSignal", "IvtInjectBody", "IvtSnapshotDepth", "IvtHome"}
raw_named, raw_seen = 0, set()
for rel, line, name, call, text, a, pos in sites:
    if not call.startswith("ivtQueueSubmit"):
        continue
    if rel.startswith("source/fg/"):
        raw_named += 1
        continue
    if name in RAW_BY_NAME:
        raw_named += 1; raw_seen.add(name)
    else:
        fail.append("%s:%d %s submits on DXVK's queue and is none of the four raw submits this check names (the marker's, the injection body's, the depth snapshot's, the home's)" % (rel, line, name))
# THE CONTROL: each of the four is found.
if raw_seen != RAW_BY_NAME:
    fail.append("CONTROL: found the raw submits of %s, the four of %s wanted; the search is broken, not the code" % (sorted(raw_seen), sorted(RAW_BY_NAME)))
print("raw submits on DXVK's queue: %d named (the marker, the injection body, the depth snapshot, the home, and frame generation's own)" % raw_named)

print("interop call sites: %d, in %d functions" % (len(sites), len({s[2] for s in sites})))
for rel, line, name, call, text, a, pos in sites:
    ok = guarded(name, text, a, pos)
    print("  %-4s %-36s %s  %s" % ("ok" if ok else "BAD", name, fg_machine_text.locate(os.path.join(ROOT, rel), line), call))
    if not ok:
        fail.append("%s:%d %s calls %s with no DT_DIRECT() ahead of it on every path" % (rel, line, name, call))

print()
if fail:
    print("FAILED, %d problem(s):" % len(fail))
    for f in fail:
        print("  -", f)
    sys.exit(1)
print("PASS")
