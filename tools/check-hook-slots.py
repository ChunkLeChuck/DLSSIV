#!/usr/bin/env python3
"""The mod's IDirect3DDevice9 slot numbers, proved against the Windows SDK's own d3d9.h.

    python tools/check-hook-slots.py

A vtable slot is the position of a method in d3d9.h's declaration of IDirect3DDevice9, and nothing else. The mod
hooks by number (`static const int kSlotX = N;` in DLSS-IV.cpp) and the device thread wraps all 119 by position, so a
wrong number is a hook on another method. This reads the SDK header the mod is built against and checks three things:

  1. every kSlot constant holds the number of the method it means. WHICH method a constant means is taken from the
     mod itself: DtRouteOriginals pairs each one with the device thread's wrapper of that method,
     DT_ROUTE_CHECK(kSlotX, origX, dt_<Method>). A constant with no such row fails too: a hook the device thread
     does not route is a bug of its own, and this is the list that would show it;
  2. the device thread's wrapper table dtDevFn[119] is dt_<Method> in the header's order;
  3. its name table dtDevName[119], which refusals and the log print, is the header's order.

UNTIL 2026-09-19 the first check lived in tools/perfproof/census_thunk_test.cpp, against a method list typed by hand
for the call census, tied to the mod by check-census-slots.py. The census was deleted with the other development
instruments; this took over its one proof of shipping code, against the header instead of a copy of it, and added
the other two.

THE CONTROL. The same checks run again on spoiled copies (one constant moved by one, two wrappers swapped, two
names swapped, one routed constant unrouted) and each copy must FAIL, so a parser that silently read nothing, or a
comparison that cannot fail, fails here instead of printing PASS.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MOD  = os.path.join(ROOT, "source", "DLSS-IV.cpp")
CORE = os.path.join(ROOT, "source", "devthread", "dt_core.h")
SYNC = os.path.join(ROOT, "source", "devthread", "dt_device_sync.h")
KITS = [r"C:\Program Files (x86)\Windows Kits\10\Include", r"C:\Program Files\Windows Kits\10\Include"]


def read(path):
    return open(path, encoding="utf-8-sig", errors="replace").read()


def code(text):
    """The text without // comments: a number or a name in a comment proves nothing."""
    return re.sub(r"//[^\n]*", "", text)


def sdk_header():
    found = []
    for k in KITS:
        found += glob.glob(os.path.join(k, "*", "shared", "d3d9.h")) + glob.glob(os.path.join(k, "*", "um", "d3d9.h"))
    def ver(p): return [int(x) for x in re.findall(r"\d+", os.path.basename(os.path.dirname(os.path.dirname(p))))]
    return max(found, key=ver) if found else None


def sdk_order(path):
    text = read(path)
    m = re.search(r"DECLARE_INTERFACE_\(\s*IDirect3DDevice9\s*,\s*IUnknown\s*\)(.*?)^\};", text, re.S | re.M)
    if not m: return []
    return [a or b for a, b in re.findall(r"STDMETHOD\(\s*(\w+)\s*\)|STDMETHOD_\(\s*[\w\s\*]+,\s*(\w+)\s*\)", code(m.group(1)))]


def table(text, decl, item):
    m = re.search(re.escape(decl) + r"\s*=\s*\{(.*?)\};", code(text), re.S)
    return re.findall(item, m.group(1)) if m else []


def check(order, slots, routes, fns, names):
    bad = []
    for k, n in sorted(slots.items(), key=lambda kv: kv[1]):
        if k not in routes: bad.append("%s = %d has no DT_ROUTE_CHECK row, so which method it means is nowhere stated and the device thread does not route it" % (k, n))
        elif not 0 <= n < len(order): bad.append("%s = %d is outside the interface" % (k, n))
        elif order[n] != routes[k]: bad.append("%s = %d means %s, and d3d9.h has %s at %d (%s is at %s)" % (
            k, n, routes[k], order[n], n, routes[k], order.index(routes[k]) if routes[k] in order else "no slot"))
    for k in sorted(routes):
        if k not in slots: bad.append("DT_ROUTE_CHECK names %s, which is not a slot constant" % k)
    for what, got in (("dtDevFn", fns), ("dtDevName", names)):
        if len(got) != len(order): bad.append("%s has %d entries and the interface has %d methods" % (what, len(got), len(order)))
        for i, (g, o) in enumerate(zip(got, order)):
            if g != o: bad.append("%s[%d] is %s and d3d9.h has %s there" % (what, i, g, o))
    return bad


def main():
    hdr = sdk_header()
    if not hdr:
        print("  no d3d9.h under %s: the proof has nothing to compare with" % " or ".join(KITS)); print("FAIL"); return 1
    order = sdk_order(hdr)
    mod = code(read(MOD))
    slots  = {m.group(1): int(m.group(2)) for m in re.finditer(r"^static const int (kSlot\w+)\s*=\s*(\d+)\s*;", mod, re.M)}
    routes = {m.group(1): m.group(2) for m in re.finditer(r"DT_ROUTE_CHECK\(\s*(kSlot\w+)\s*,\s*\w+\s*,\s*dt_(\w+)\s*\)", mod)}
    fns    = table(read(SYNC), "static void* const dtDevFn[119]", r"&dt_(\w+)")
    names  = table(read(CORE), "static const char* const dtDevName[119]", r"\"(\w+)\"")
    print("  %s: %d methods" % (hdr, len(order)))
    print("  %d slot constants, %d routed, %d wrappers, %d names" % (len(slots), len(routes), len(fns), len(names)))
    if len(order) != 119 or not slots or not routes or not fns or not names:
        print("  something was not found at all: a parser read nothing"); print("FAIL"); return 1

    bad = check(order, slots, routes, fns, names)
    for b in bad: print("  " + b)

    # THE CONTROL: four spoiled copies, each of which must be caught.
    k0 = sorted(slots)[0]
    spoiled = {
        "one constant moved by one":    (order, dict(slots, **{k0: slots[k0] + 1}), routes, fns, names),
        "two wrappers swapped":         (order, slots, routes, [fns[1], fns[0]] + fns[2:], names),
        "two names swapped":            (order, slots, routes, fns, names[:40] + [names[41], names[40]] + names[42:]),
        "one routed constant unrouted": (order, slots, {k: v for k, v in routes.items() if k != k0}, fns, names),
    }
    missed = [what for what, args in spoiled.items() if not check(*args)]
    for what in missed: print("  THE CONTROL DID NOT FAIL: %s went unnoticed" % what)
    print("  control: %d of %d spoiled copies caught" % (len(spoiled) - len(missed), len(spoiled)))
    ok = not bad and not missed
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
