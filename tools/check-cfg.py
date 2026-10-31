#!/usr/bin/env python3
"""Five things about the cfg that nothing else can prove, checked mechanically.

Run it after touching IvtCfg, IvtParseCfg, IvtClampCfg, either key macro, or
docs/CONFIG.md:

    python tools/check-cfg.py

Exit code 0 means all five hold. Anything else prints what broke and why it
matters.

  1. THE POSITIONAL INITIALISER LINES UP WITH THE STRUCT. `ivtCfg` is
     initialised positionally, and two defaults have shipped wrong in this
     project's history because a field was inserted in the middle. Field count
     and field-vs-literal type are both checked; a float value against an int
     field, or the reverse, is the shape that mistake takes.

  2. EVERY int FIELD IS IN EXACTLY ONE KEY LIST. A field in neither
     IVT_LIVE_INT_KEYS nor IVT_STATIC_INT_KEYS is not "static", it is INVISIBLE:
     the live tick reports neither an applied change nor a "needs a relaunch",
     and IvtReportClamps cannot name it when the clamp rewrites it. A field in
     both is applied and then declared un-appliable in the same second.

  3. EVERY KEY THE PARSER ACCEPTS IS IN docs/CONFIG.md. The 2026-09-09 audit
     found `debug_gap`, `diag` and `hdr_ui_white` live, clamped and undocumented,
     because the check that existed ran the other way: it compared documented
     rows against code defaults, so a key with no row at all was invisible to
     it. This runs BOTH directions.

  4. THE SHIPPED FILE sets only keys a RELEASE build accepts (the development fences removed), each at its compiled
     default or a recorded departure, in plain ASCII; three spoiled copies and a development key are each caught.

  5. THE RELEASE VIEW (the final audit, 2026-09-30, cfgAndList-check-cfg-gaps). The initialiser is counted again with the
     `#if DLSSIV_DEV` blocks removed, as a release build sees it: a value on the other side of a fence from its field
     shifts every later default of a release with no compile error. And every int field a release build carries must be
     set by a release parse line, or be on DEV_PARSED: a field nothing can set in a release is dead weight the clamps and
     the live tick still walk. Each check has a control that must fail.
"""

import io, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
H = os.path.join(ROOT, "source", "transport", "DlssIvTransport.h")
D = os.path.join(ROOT, "docs", "CONFIG.md")
FGC = os.path.join(ROOT, "source", "fg", "fg_client.h")   # frame generation's product key parser (product_keys says why)

# `backend` is the struct field and the wire term (FEED_ACK_BACKEND_FSR,
# --backend=); `upscaler` is the player-facing name for the same field and has
# its own heading in CONFIG.md, which explains both. Documented, just not under
# a heading of its own, and renaming the protocol to match the document was
# weighed and refused.
DOC_EXEMPT = {"backend"}

# THE SHIPPED FILE (final audit 2026-09-19, F8.3). prove-all.py said this tool proved the shipped cfg and no tool in the
# repository opened it: twelve keys were deleted that day and a leftover in DLSS-IV.cfg would have printed PASS.
C = os.path.join(ROOT, "DLSS-IV.cfg")
KEY_FIELD = {"upscaler": "backend"}                 # the player's name for the field, see DOC_EXEMPT
# A shipped value that is NOT the compiled default, on purpose, with where it is decided. Anything else must be equal.
SHIPPED_DEPARTS = {"hdr_paper_white": "203"}       # BT.2408 reference white; the compiled 0 means "the OS SDR white level" (CONFIG.md)
DEFAULTS = {}
# RELEASE FIELDS WHOSE PARSE LINE IS DEVELOPMENT-ONLY, ON PURPOSE (the owner, 2026-09-30, cfgAndList-hidden-keys): answered
# diagnostic levers a release build no longer reads, their fields and clamps kept so the struct's layout does not move.
# (Since the deep strip of 2026-10-25 ten of them, all but flags, rs_ds and mv_glass, are parsed by NO build: fields held at
# the shipped value; their fields go when the release's base is next re-pinned.)
DEV_PARSED = {"flags", "rs_ds", "mv_glass"}   # (stage, reset_on_cut, jitter_hold, jitter_phases, hdr_lut_pivot, dedither, steady_depth, rain_layer,
                                              #  corona_hold and quad_uv, parsed by no build after round 8's strip S1, left IvtCfg in its S4)


def release_view(text):
    """The text as a release build compiles it: every `#if DLSSIV_DEV` ... `#endif` block removed (the fences here do not nest),
    and every `#if DLSSIV_FG` block too (2026-10-05, phase 2 stage 7): frame generation's switch follows DLSSIV_DEV, so a
    shipped release compiles neither, and a key behind either fence is not a key a release accepts."""
    return re.sub(r"(?ms)^[ \t]*#if DLSSIV_(?:DEV|FG)\b.*?^[ \t]*#endif[^\n]*\n?", "", text)


def parse_body(text):
    a = text.index("static void IvtParseCfg(")
    return text[a:a + text[a:].index("\nstatic void IvtClampCfg(")]

h = io.open(H, "r", encoding="utf-8").read()
d = io.open(D, "r", encoding="utf-8").read()
fail = []


def check_initialiser():
    a = h.index("int transport;")
    b = a + h[a:].index("; };") + 1                            # the struct's last field, whichever it is (it was hard-coded to `threaded` until 2026-09-18)
    body = re.sub(r"//[^\n]*", "", h[a:b])
    # A FENCED KEY (2026-09-19). A development-only cfg key is fenced with #if inside the struct
    # and the same #if inside the initialiser. This check used to read the directive as a
    # declaration and fail the build, which blocked every such fence before it was written.
    # Now: the directives are lifted out of both, the two lists must be IDENTICAL and in the
    # same order (a field fenced in one place and not the other is the bug worth catching),
    # and the fields and values are then counted as the development build sees them.
    PRE = re.compile(r"^[ \t]*#[^\n]*", re.M)
    body_pre = [p.strip() for p in PRE.findall(body)]
    body = PRE.sub("", body)
    fields = []
    for decl in body.split(";"):
        decl = decl.strip()
        if not decl:
            continue
        parts = decl.split()
        if parts[0] not in ("int", "float"):
            fail.append("unexpected declaration in IvtCfg: %s" % decl[:70])
            return None
        for nm in " ".join(parts[1:]).split(","):
            if nm.strip():
                fields.append((parts[0], nm.strip()))
    init = re.search(r"static IvtCfg   ivtCfg\s*=\s*\{(.*?)\};", h, re.S).group(1)
    init = re.sub(r"//[^\n]*", "", init)
    init_pre = [p.strip() for p in PRE.findall(init)]
    init = PRE.sub("", init)
    if body_pre != init_pre:
        fail.append("IvtCfg's fences and its initialiser's fences differ: %r against %r" % (body_pre, init_pre))
    vals = [v.strip() for v in init.split(",") if v.strip()]
    print("1. initialiser : %d struct fields, %d values" % (len(fields), len(vals)))
    if len(fields) != len(vals):
        fail.append("IvtCfg has %d fields but the positional initialiser has %d values"
                    % (len(fields), len(vals)))
        return fields
    DEFAULTS.update({nm: v for (ty, nm), v in zip(fields, vals)})
    for i, ((ty, nm), v) in enumerate(zip(fields, vals)):
        literal_is_float = v.endswith("f") or "." in v
        if (ty == "float") != literal_is_float:
            fail.append("slot %d: %s %s takes the literal %s" % (i, ty, nm, v))
    if not fail:
        print("   every field's type matches its value's literal form")
    return fields


def count_release(text):
    """(fields, values) as a release build sees IvtCfg and its initialiser."""
    a = text.index("int transport;")
    b = a + text[a:].index("; };") + 1
    body = release_view(re.sub(r"//[^\n]*", "", text[a:b]))
    nf = 0
    for decl in body.split(";"):
        decl = decl.strip()
        if decl:
            nf += len([n for n in " ".join(decl.split()[1:]).split(",") if n.strip()])
    init = re.search(r"static IvtCfg   ivtCfg\s*=\s*\{(.*?)\};", text, re.S).group(1)
    init = release_view(re.sub(r"//[^\n]*", "", init))
    return nf, len([v for v in init.split(",") if v.strip()])


def release_unset(text):
    """Release int fields no release parse line assigns, DEV_PARSED aside."""
    a = text.index("int transport;")
    b = a + text[a:].index("; };") + 1
    body = release_view(re.sub(r"//[^\n]*", "", text[a:b]))
    ints = []
    for decl in body.split(";"):
        parts = decl.split()
        if parts and parts[0] == "int":
            ints += [n.strip() for n in " ".join(parts[1:]).split(",") if n.strip()]
    assigned = set(re.findall(r"c->([A-Za-z0-9_]+)\s*=", release_view(parse_body(text))))
    return [f for f in ints if f not in assigned and f not in DEV_PARSED]


def check_release_view():
    nf, nv = count_release(h)
    print("5. release view: %d struct fields, %d values" % (nf, nv))
    if nf != nv:
        fail.append("as a release build sees it, IvtCfg has %d fields and the initialiser %d values" % (nf, nv))
    unset = release_unset(h)
    if unset:
        fail.append("release int fields no release parse line sets (and not DEV_PARSED): %s" % unset)
    # THE CONTROLS: a value moved inside the fence must break the release count, and a parse line taken out of the release
    # view must name its field.
    i0 = h.index("static IvtCfg   ivtCfg")
    fence = h.index("#if DLSSIV_DEV", i0)
    c1 = h.rindex(",", i0, fence)                     # the comma after the last value before the fence
    c0 = h.rindex(",", i0, c1)                        # the one before it: h[c0+1:c1] is that value
    val = h[c0 + 1:c1].strip()
    spoiled = h[:c0 + 1] + h[c1 + 1:fence] + h[fence:].replace("#if DLSSIV_DEV", "#if DLSSIV_DEV\n    " + val + ",", 1)
    mf, mv = count_release(spoiled)
    if mf == mv:
        fail.append("CONTROL: a value moved inside the initialiser's fence was not caught (%d fields, %d values)" % (mf, mv))
    dropped = h.replace('c->unpin = iv;', 'c->unpin_gone = iv;', 1)
    if "unpin" not in release_unset(dropped):
        fail.append("CONTROL: a release field whose parse line was taken out was not named")
    if not any("release" in f or "CONTROL" in f for f in fail):
        print("   the release initialiser lines up, every release int field has a release parse line (%d on DEV_PARSED);" % len(DEV_PARSED))
        print("   and a value moved across the fence and a parse line taken out were each caught")


def key_list(macro, view="dev"):
    m = re.search(r"#define %s\(X\)(.*?)(?=\n(?:[^\s/]|//))" % macro, h, re.S)
    if not m:
        fail.append("could not find %s" % macro)
        return []
    # A KEY CAN SIT IN ONE LIST IN A DEVELOPMENT BUILD AND IN THE OTHER IN A RELEASE (2026-10-02: jitter_phases is live in a
    # development build and static in a release, IVT_DEV_ONLY in one list and IVT_REL_ONLY in the other). `view` drops the
    # entries the other build's macro wraps, so each build is checked as the compiler sees it.
    body = m.group(1)
    drop = "IVT_REL_ONLY" if view == "dev" else "IVT_DEV_ONLY"
    body = re.sub(r"%s\(X\([A-Za-z0-9_]+\)\)" % drop, "", body)
    return re.findall(r"X\(([A-Za-z0-9_]+)\)", body)


def check_key_lists(fields):
    if not fields:
        return
    ints = [n for ty, n in fields if ty == "int"]
    live, static = key_list("IVT_LIVE_INT_KEYS"), key_list("IVT_STATIC_INT_KEYS")
    print("2. key lists   : %d int fields, %d live + %d static" % (len(ints), len(live), len(static)))
    for f in ints:
        n = (f in live) + (f in static)
        if n != 1:
            fail.append("%s is in %d key list(s); it must be in exactly one" % (f, n))
    # the release's view: no key twice, and a key a release-only entry names is there exactly once
    rlive, rstatic = key_list("IVT_LIVE_INT_KEYS", "rel"), key_list("IVT_STATIC_INT_KEYS", "rel")
    rel_only = set(re.findall(r"IVT_REL_ONLY\(X\(([A-Za-z0-9_]+)\)\)", h))
    for f in sorted(set(rlive + rstatic) | rel_only):
        n = rlive.count(f) + rstatic.count(f)
        if n != 1:
            fail.append("%s is in %d key list(s) of a RELEASE build; it must be in exactly one" % (f, n))
    for k in sorted(set(live + static) - set(ints)):
        fail.append("%s is in a key list but is not an int field of IvtCfg" % k)
    if not any("key list" in f for f in fail):
        print("   every int field is in exactly one list")


def product_keys():
    """FRAME GENERATION'S PRODUCT KEYS (2026-10-05, phase 2 stage 7): IvtParseCfg hands a key it does not know to FgxParseKey
    (source/fg/fg_client.h, builds with DLSSIV_FG only), which names each key it takes in a `_stricmp(k, "...")`. They are
    accepted keys, so rule 3 holds them to a row in CONFIG.md. The shipped cfg's check does NOT accept them (release_view
    drops the fence, and the names are not in IvtParseCfg at all): a shipped file must not name a key no release reads."""
    if not os.path.exists(FGC):
        return set()
    r = io.open(FGC, "r", encoding="utf-8").read()
    m = re.search(r"static bool FgxParseKey\(.*?\n\}", r, re.S)
    if not m:
        fail.append("fg_client.h exists and its FgxParseKey could not be found")
        return set()
    keys = set(re.findall(r'_stricmp\(k,\s*"([a-z0-9_]+)"\)', m.group(0)))
    if not keys:
        fail.append("fg_client.h's FgxParseKey names no key: the reader found too little to have read it")
    # (THE ONE KEY WHOSE VALUE WAS TEXT, fg_host_args, FgxParseTextKey: 2026-10-07 to round 8's deep strip)
    if "FgxParseKey(k, v)" not in release_view_fg(parse_body(h)):
        fail.append("fg_client.h has FgxParseKey and IvtParseCfg's chain does not call it behind `#if DLSSIV_FG`")
    return keys


# THE SHAPE 1.1 SHIPS READS TWO KEYS MORE (round 8, 2026-10-25): build.ps1 -Ship, package.ps1's default, compiles FgxParseKey
# (source/fg/fg_client.h) with DLSSIV_FG (armed) and without DLSSIV_DEVLOG, and there it takes `fg` and `fg_min_fps`,
# each a field of the zero-initialised fgx (0: frame generation off, the owner's default for 1.1; 0: the floor by display). The
# shipped file carries both at 0. ship_fg_keys reads the parser as that shape compiles it and must find exactly these.
SHIP_FG_DEFAULTS = {"fg": "0", "fg_min_fps": "0"}


def ship_fg_keys():
    """The keys FgxParseKey takes in the shape 1.1 ships: its body with every `#if DLSSIV_DEVLOG` block taken out, nesting
    counted (the development block may hold blocks of its own)."""
    if not os.path.exists(FGC):
        return set()
    r = io.open(FGC, "r", encoding="utf-8").read()
    m = re.search(r"static bool FgxParseKey\(.*?\n\}", r, re.S)
    if not m:
        return set()
    kept, stack = [], []
    for line in m.group(0).split("\n"):
        s = line.strip()
        if s.startswith("#if"):
            stack.append(s.startswith("#if DLSSIV_DEVLOG") or s.startswith("#if DLSSIV_DEV ") or s == "#if DLSSIV_DEV")
            continue
        if s.startswith("#endif"):
            if stack: stack.pop()
            continue
        if s.startswith("#el"):
            continue
        if not any(stack):
            kept.append(line)
    return set(re.findall(r'_stricmp\(k,\s*"([a-z0-9_]+)"\)', "\n".join(kept)))


def release_view_fg(text):
    """The parser's body with ONLY the `#if DLSSIV_FG` blocks kept: what frame generation's switch adds to it."""
    return "\n".join(re.findall(r"(?ms)^[ \t]*#if DLSSIV_FG\b.*?^[ \t]*#endif[^\n]*", text))


def check_documented():
    rig = product_keys()   # (the frame generation rig's own keys, fg_rig.h's FgRigKeyDef, went with the rig in round 8's deep strip)
    accepted = sorted(set(re.findall(r'_stricmp\(k,\s*"([A-Za-z0-9_]+)"\)', h)) | rig)
    documented = (set(re.findall(r"^\|\s*`([A-Za-z0-9_]+)`\s*\|", d, re.M))
                  | set(re.findall(r"^###\s+`([A-Za-z0-9_]+)`", d, re.M)))
    print("3. documented  : parser accepts %d keys (%d of them frame generation's product's), CONFIG.md carries %d"
          % (len(accepted), len(rig), len(documented)))
    for k in accepted:
        if k not in documented and k not in DOC_EXEMPT:
            fail.append("%s is accepted by IvtParseCfg and has no row or heading in docs/CONFIG.md" % k)
    for k in sorted(documented - set(accepted)):
        fail.append("%s has a row in docs/CONFIG.md and IvtParseCfg does not accept it" % k)
    if not any("CONFIG.md" in f for f in fail):
        print("   every accepted key is documented, and every documented key is accepted")


def shipped_problems(raw):
    """Every problem with a shipped cfg given as bytes. Separate from the printing so the control can call it."""
    out = []
    if raw.startswith(b"\xef\xbb\xbf"):
        out.append("DLSS-IV.cfg starts with a byte-order mark")
    if any(b >= 0x80 for b in raw):
        out.append("DLSS-IV.cfg has a non-ASCII byte (a player's editor and the parser may not agree on it)")
    accepted = set(re.findall(r'_stricmp\(k,\s*"([A-Za-z0-9_]+)"\)', release_view(parse_body(h))))   # what a RELEASE accepts
    accepted |= set(SHIP_FG_DEFAULTS)                                                                  # and the shape 1.1 ships (round 8)
    live = []
    for m in re.finditer(r"(?m)^[ \t]*(#?)[ \t]*([a-z][a-z0-9_]*)[ \t]*=[ \t]*([^\s#]+)[ \t]*\r?$", raw.decode("ascii", "replace")):
        commented, key, val = m.group(1) == "#", m.group(2), m.group(3)
        if key not in accepted:
            out.append("DLSS-IV.cfg %s `%s=`, which IvtParseCfg does not accept" % ("offers (commented)" if commented else "sets", key))
            continue
        if commented:
            continue
        live.append(key)
        field = KEY_FIELD.get(key, key)
        if key in SHIP_FG_DEFAULTS:
            if val != SHIP_FG_DEFAULTS[key]:
                out.append("DLSS-IV.cfg ships `%s=%s` and the compiled default is %s" % (key, val, SHIP_FG_DEFAULTS[key]))
            continue
        if field not in DEFAULTS:
            out.append("DLSS-IV.cfg sets `%s` and IvtCfg has no field `%s` to take its default from" % (key, field))
            continue
        want = SHIPPED_DEPARTS.get(key, DEFAULTS[field].rstrip("f"))
        try:
            same = float(val) == float(want)
        except ValueError:
            same = False
        if not same:
            out.append("DLSS-IV.cfg ships `%s=%s` and the %s is %s" % (key, val, "recorded departure" if key in SHIPPED_DEPARTS else "compiled default", want))
    if len(live) != len(set(live)):
        out.append("DLSS-IV.cfg sets a key twice: %s" % sorted(k for k in set(live) if live.count(k) > 1))
    return out, live


def check_shipped():
    raw = io.open(C, "rb").read()
    probs, live = shipped_problems(raw)
    print("4. shipped cfg : %d keys set (%s)" % (len(live), " ".join(live)))
    fail.extend(probs)
    if len(live) < 5:
        fail.append("DLSS-IV.cfg sets only %d keys: the reader found too little to have read the file" % len(live))
    # THE CONTROL: the same reader on three spoiled copies must name each spoil, or a PASS above means nothing.
    spoils = ((b"\r\nrs_scan=1\r\n", "does not accept"), (b"\r\n#watchdog=1\r\n", "does not accept"), (b"\r\nhdr_engine_name=0\r\n", "compiled default"),   # a key the file does not carry (threaded=0 was the spoil until 1.0.8 put threaded in the file, rename_sdr=0 until 1.1 put that one in)
              (b"\r\nframes_csv=0\r\n", "does not accept"),   # a development key until the strip of 2026-10-25: a release player would get the unknown-key line
              (b"\r\nfg_fixed=1\r\n", "does not accept"))     # frame generation's development key: only the armed development build reads it
    # (round 8) frame generation's own line turned on: the shape 1.1 ships reads `fg`, and the file ships it off
    on = raw.replace(b"\r\nfg=0\r\n", b"\r\nfg=1\r\n", 1)
    got, _ = shipped_problems(on)
    if on == raw or len(got) != len(probs) + 1 or "compiled default is 0" not in got[-1]:
        fail.append("CONTROL: a shipped cfg with `fg=1` for `fg=0` was not caught (%r)" % (got[len(probs):],))
    sk = ship_fg_keys()
    if sk != set(SHIP_FG_DEFAULTS):
        fail.append("FgxParseKey takes %s in the shape 1.1 ships (its development blocks taken out), and SHIP_FG_DEFAULTS names %s"
                    % (sorted(sk), sorted(SHIP_FG_DEFAULTS)))
    for extra, needle in spoils:
        got, _ = shipped_problems(raw + extra)
        if len(got) != len(probs) + 1 or needle not in got[-1]:
            fail.append("CONTROL: a shipped cfg spoiled with %r was not caught (%r)" % (extra.strip(), got[len(probs):]))
    if not any("DLSS-IV.cfg" in f or "CONTROL" in f for f in fail):
        print("   every key it names is accepted, every value is the compiled default or a recorded departure, plain ASCII;")
        print("   and three spoiled copies (a deleted key set, a deleted key offered, a changed default) were each caught")


check_key_lists(check_initialiser())
check_documented()
check_shipped()
check_release_view()

print()
if fail:
    print("FAILED, %d problem(s):" % len(fail))
    for f in fail:
        print("  -", f)
    sys.exit(1)
print("PASS")
