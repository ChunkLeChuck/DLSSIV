"""THE NOTICE'S RULES, PROVEN (2026-10-11). Not shipped; no Direct3D, so prove-all can run it.

    python tools\\test-notice-rules\\prove.py

1. THE WIRING, as text, each check inside the function it is about: IvtWarn hands its line on once, IvtFatalNote says it
   once, the frame's end calls the notice before the HDR encode, the start-up thread makes and sets the two copies'
   event, the cfg's complaints are scrubbed first, notice.h is the file's last line, and NEITHER THE CFG'S PARSER NOR
   IvtNoteBadCfg NAMES THE NOTICE (one line in either moved the release's hot data).
2. EVERY WARNING THE SOURCE CAN WRITE, AGAINST THE TABLE. Each IvtWarn call's format is read from the source, filled
   with stand-in values and given to the rules' own text (cut from source\\notice.h, built as nr.exe); the one with a
   reason inside (HDR not on) is filled with every reason the source can give it, the one about DLSS with every cause.
   What comes back is held against what the owner approved on 2026-10-10: which warnings are notices at all, and the
   exact sentence of each. A warning added later that begins like one of these, or a reason added to the list, shows
   here before it shows on a player's screen. INTEL XeSS (2026-10-28, XeSS X3): the warning that automatic fell to FSR on
   an Intel card is filled with every reason IvtXessWhy (DLSS-IV.cpp's end) can give it, and NONE is a notice: the owner
   approved none for XeSS, so a rule that would show one fails here (control 4). (fg18-fix, F6: the same warning is written where
   automatic falls from XeSS to FSR at a build, IvtXessFell, with the same format: two calls, one table.)
3. THE QUEUE, THE FADE, THE DID-NOT-START TEXTS, THE CFG OVERRIDE AND THE TWO COPIES' EVENT (nr.exe rules).
4. THREE CONTROLS: the rules' text spoiled in one place each must fail part 2.
"""
import os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
SRC = os.path.join(ROOT, "source")
fails = []

HDR_GENERIC = "DLSS-IV: HDR is not on this session."
HDR_BORDERLESS = "DLSS-IV: HDR is not on. Choose Borderless in the game's graphics options."
DLSS_DRIVER = "DLSS-IV: DLSS could not start, so FSR is running. Update the NVIDIA driver."
DLSS_FILE = "DLSS-IV: DLSS could not start, so FSR is running. Copy the mod's DLSSIV folder again."
OTHER_MOD = "DLSS-IV: Another mod got in the way at start. Change the resolution once, or restart the game."
WRONGLY = "DLSS-IV: The game may render wrongly. Set threaded=0 in DLSS-IV.cfg."
RESTART = "That change in DLSS-IV.cfg needs a restart of the game."
TWO = "DLSS-IV: Two copies of DLSS-IV.asi are loaded. Remove one."
MORE_WARN = "More in DLSS-IV-WARNINGS.txt, next to GTAIV.exe"
MORE_FATAL = "The reason is in DLSS-IV-DID-NOT-START.txt, next to GTAIV.exe"
P_HDR = "hdr=1 is set, but HDR is not on (decided at %s): %s"
P_DLSS = "upscaler=-1 (automatic) is running FSR, not DLSS: %s"
P_XESS = "upscaler=-1 (automatic) is running FSR, not XeSS: %s"   # 2026-10-28, XeSS X3: no notice for any of its reasons


def check(ok, what):
    print("  %s  %s" % ("ok  " if ok else "FAIL", what))
    if not ok: fails.append(what)
    return ok


def read(rel):
    return open(os.path.join(SRC, rel), "rb").read().decode("utf-8", errors="replace").replace("\r\n", "\n")


def tokens(text, i, stop_at_arg_end):
    """C tokens from text[i:]: ('S', string) for a string literal, ('O', text) for anything else that is not white space
    or a comment. With stop_at_arg_end, stops at the comma or the closing parenthesis that ends the first argument."""
    out = []; depth = 0; n = len(text)
    while i < n:
        c = text[i]
        if c == '/' and text[i + 1:i + 2] == '/':
            i = text.index("\n", i); continue
        if c == '/' and text[i + 1:i + 2] == '*':
            i = text.index("*/", i) + 2; continue
        if c == '"':
            j = i + 1; buf = []
            while text[j] != '"':
                if text[j] == '\\':
                    buf.append({'n': '\n', 't': '\t', '"': '"', '\\': '\\'}.get(text[j + 1], text[j + 1])); j += 2
                else:
                    buf.append(text[j]); j += 1
            out.append(('S', "".join(buf))); i = j + 1; continue
        if c == "'":
            j = text.index("'", i + 2 if text[i + 1] == '\\' else i + 1); out.append(('O', text[i:j + 1])); i = j + 1; continue
        if c.isspace(): i += 1; continue
        if stop_at_arg_end:
            if c in "([{": depth += 1
            elif c in ")]}":
                if depth == 0: return out, i
                depth -= 1
            elif c == ',' and depth == 0: return out, i
        m = re.match(r"[A-Za-z_][A-Za-z_0-9]*", text[i:])
        if m: out.append(('O', m.group())); i += len(m.group()); continue
        out.append(('O', c)); i += 1
    return out, i


def literal_groups(toks):
    """Runs of adjacent string literals, joined; an ALL-CAPS name between two literals is a macro's text and stands as X."""
    groups = []; cur = None
    for k, (kind, val) in enumerate(toks):
        if kind == 'S':
            cur = (cur or "") + val
        elif cur is not None and re.fullmatch(r"[A-Z][A-Z0-9_]+", val) and k + 1 < len(toks) and toks[k + 1][0] == 'S':
            cur += "X"
        else:
            if cur is not None: groups.append(cur)
            cur = None
    if cur is not None: groups.append(cur)
    return groups


def fill(fmt, strings=None):
    strings = list(strings or [])
    def one(m):
        if m.group(0) == "%%": return "%"
        t = m.group(0)[-1]
        if t == 's': return strings.pop(0) if strings else "S"
        if t == 'f': return "0.50"
        return "7"
    return re.sub(r"%%|%[-+ 0#]*\d*(?:\.\d+)?(?:ll|l|h)?[sdiuxXfc]", one, fmt)


def warn_formats():
    """(file, line, format) for every IvtWarn call in the source whose first argument holds a string literal."""
    out = []; variable = []
    for dirpath, _, names in os.walk(SRC):
        for n in names:
            if not n.endswith((".cpp", ".h")) or n == "notice.h": continue
            rel = os.path.relpath(os.path.join(dirpath, n), SRC)
            text = read(rel)
            for m in re.finditer(r"\bIvtWarn\(", text):
                head = text[text.rfind("\n", 0, m.start()) + 1:m.start()]
                if "static void" in head or "//" in head: continue            # the prototype, the definition, a comment
                toks, _ = tokens(text, m.end(), True)
                if toks and toks[0] == ('O', '_Printf_format_string_'): continue
                groups = literal_groups(toks)
                line = text.count("\n", 0, m.start()) + 1
                if not groups: variable.append("%s:%d" % (rel, line))
                for g in groups: out.append((rel, line, g))
    return out, variable


def region_literals(rel, start, end):
    text = read(rel)
    a = text.index(start); b = text.index(end, a)
    toks, _ = tokens(text[a:b], 0, False)
    return literal_groups(toks)


def expected(fmt, line):
    """What the owner approved (2026-10-10), by the warning's own format: the notice's first line, or None."""
    if fmt.startswith("hdr=1 is set, but the back buffer came back as ") or fmt.startswith("hdr=1 is set and Windows HDR is on, but the display surface does not accept") \
            or fmt.startswith("hdr=1: HDR engaged, but after "): return HDR_GENERIC
    if " scene buffer the render scale needs could not be created" in fmt: return "DLSS-IV: The picture is wrong. Set %s=0 in DLSS-IV.cfg." % line.split("=")[0]
    if fmt.startswith("Another mod (") and "replaced DLSS-IV's Direct3D hook as the game started" in fmt: return OTHER_MOD
    if fmt.startswith("DXVK could not end a state block recording"): return WRONGLY
    if fmt.startswith("DLSS-IV.cfg was edited while the game runs: ") and "needs a relaunch" in fmt: return RESTART
    return None


def cut_rules(text):
    a = text.index("// ---- NOTICE RULES START"); b = text.index("// ---- NOTICE RULES END ----")
    return text[a:b]


def build(cut, exe=None):
    open(os.path.join(HERE, "nr_cut.h"), "w", encoding="utf-8", newline="\n").write(cut)
    a = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", os.path.join(HERE, "build.ps1")] + (["-Out", exe] if exe else [])
    r = subprocess.run(a, capture_output=True, text=True)
    os.remove(os.path.join(HERE, "nr_cut.h"))
    return r.returncode == 0, r.stdout[-2000:] + r.stderr[-1000:]


def ask(exe, lines, work):
    inp = os.path.join(work, "lines.txt"); outp = os.path.join(work, "out.txt")
    open(inp, "w", encoding="latin-1", newline="\n").write("\n".join(lines) + "\n")
    r = subprocess.run([exe, "warn", inp, outp], capture_output=True, text=True)
    got = open(outp, encoding="latin-1").read().split("\n")[:-1] if r.returncode == 0 else []
    return [None if g == "-" else g.split("\t") for g in got]


def part2(exe, work, quiet=False):
    """Returns the list of what did not hold (empty: all of it held)."""
    bad = []
    formats, variable = warn_formats()
    reasons = region_literals("hdr_encode.h", "const char* refuse = nullptr;", "// THE FULLSCREEN GATE IS A WINDOWS FACT")   # the refusals alone: the log lines after them are not reasons
    causes = region_literals(os.path.join("transport", "DlssIvTransport.h"), "const char* noDlss = ", "if (ivtCfg.backend == 0 && IVT_MODE != 1 && dlssTried")
    xess = region_literals("DLSS-IV.cpp", "static const char* IvtXessWhy(", "static const char* IvtXessFix(")   # its reasons, the five returns
    plain = [(rel, ln, f) for rel, ln, f in formats if f not in (P_HDR, P_DLSS, P_XESS)]
    lines = [fill(f) for _, _, f in plain]
    extra = [("quality=1 is set, but the 1280x720 scene buffer the render scale needs could not be created (video memory), so the picture is wrong: set it to 0", "DLSS-IV: The picture is wrong. Set quality=0 in DLSS-IV.cfg."),
             ("custom=50 is set, but the 1920x1080 scene buffer the render scale needs could not be created (video memory), so the picture is wrong: set it to 0", "DLSS-IV: The picture is wrong. Set custom=0 in DLSS-IV.cfg.")]
    hdr_lines = [fill(P_HDR, ["CreateDevice", r]) for r in reasons]
    dlss_lines = [fill(P_DLSS, [c]) for c in causes]
    xess_lines = [fill(P_XESS, [x]) for x in xess]
    got = ask(exe, lines + [e[0] for e in extra] + hdr_lines + dlss_lines + xess_lines, work)
    if len(got) != len(lines) + len(extra) + len(hdr_lines) + len(dlss_lines) + len(xess_lines): return ["nr.exe did not answer every line"]
    g_plain, rest = got[:len(lines)], got[len(lines):]
    g_extra, rest = rest[:len(extra)], rest[len(extra):]
    g_hdr, rest = rest[:len(hdr_lines)], rest[len(hdr_lines):]
    g_dlss, g_xess = rest[:len(dlss_lines)], rest[len(dlss_lines):]
    made = 0
    for (rel, ln, f), line, g in zip(plain, lines, g_plain):
        want = expected(f, line)
        if want: made += 1
        if (g[0] if g else None) != want: bad.append("%s:%d: %r gives %r, approved %r" % (rel, ln, f[:60], g[0] if g else None, want))
        if g and (g[1] != MORE_WARN or g[2] != "10"): bad.append("%s:%d: second line or seconds %r" % (rel, ln, g[1:]))
    for (line, want), g in zip(extra, g_extra):
        if (g[0] if g else None) != want: bad.append("the render size's line gives %r, approved %r" % (g[0] if g else None, want))
    h = [g[0] if g else None for g in g_hdr]
    want_h = [None if "Windows HDR is off" in r else HDR_BORDERLESS if "exclusive fullscreen" in r else HDR_GENERIC for r in reasons]
    if h != want_h: bad.append("HDR's reasons give %r" % h)
    if not (len(reasons) >= 6 and want_h.count(None) == 1 and want_h.count(HDR_BORDERLESS) == 1): bad.append("HDR's reasons as read from the source: %d, %d silent, %d Borderless" % (len(reasons), want_h.count(None), want_h.count(HDR_BORDERLESS)))
    d = [g[0] if g else None for g in g_dlss]
    want_d = [DLSS_DRIVER if "NVIDIA driver is older" in c else DLSS_FILE if "nvngx_dlss.dll is not there" in c else None for c in causes]
    if d != want_d: bad.append("DLSS's causes give %r" % d)
    if not (len(causes) >= 6 and want_d.count(DLSS_DRIVER) == 1 and want_d.count(DLSS_FILE) == 1): bad.append("DLSS's causes as read from the source: %d, %d driver, %d file" % (len(causes), want_d.count(DLSS_DRIVER), want_d.count(DLSS_FILE)))
    # XeSS (2026-10-28, X3): the owner approved no notice for it, so every reason gives none
    x = [g[0] if g else None for g in g_xess]
    if x != [None] * len(xess): bad.append("XeSS's reasons give %r, approved none" % x)
    if not (len(xess) == 5 and sum(1 for r in xess if "libxess.dll is not there" in r) == 1 and sum(1 for r in xess if "runtime 14.40" in r) == 2):
        bad.append("XeSS's reasons as read from the source: %d (five: the file, the card, the runtime twice, the rest)" % len(xess))
    if not quiet:
        print("   %d IvtWarn formats in %d calls' worth of source (and %d calls whose text is a variable: %s); %d reasons for HDR, %d causes for DLSS, %d reasons for XeSS" % (
            len(formats), len(set((r, l) for r, l, _ in formats)), len(variable), ", ".join(variable) or "none", len(reasons), len(causes), len(xess)))
        check(len(formats) >= 40 and sum(1 for _, _, f in formats if f == P_HDR) == 1 and sum(1 for _, _, f in formats if f == P_DLSS) == 1
              and sum(1 for _, _, f in formats if f == P_XESS) == 2,   # (fg18-fix, F6: the XeSS one twice, the handshake's fall and a build's, one format filled with every reason)
              "the source's warnings were found: %d formats, the HDR one and the DLSS one once each, the XeSS one twice (control)" % len(formats))
        check(made == 8, "eight plain warnings are notices (three of HDR, the render size's buffer, another mod at start, the start-up check, the cfg's two replies): %d" % made)
        longest = max([g for g in got if g], key=lambda g: len(g[0]))
        check(all(len(g[0]) < 127 and len(g[1]) < 95 for g in got if g), "no notice is cut short (the longest first line is %d characters)" % len(longest[0]))
    return bad


def main():
    print("1. the wiring")
    cpp = read("DLSS-IV.cpp"); hdr = read(os.path.join("transport", "DlssIvTransport.h")); nh = read("notice.h")
    def body(text, head):
        a = text.index(head); return text[a:text.index("\n}\n", a)]
    def code(s): return "\n".join(l.split("//")[0] for l in s.splitlines())
    w = code(body(cpp, "static void IvtWarn(_Printf_format_string_ const char* fmt, ...)\n{"))
    check("vsnprintf(line" in w and w.count("NoticeFromWarn(line);") == 1 and w.index("NoticeFromWarn(line);") > w.index("vsnprintf(line"), "IvtWarn hands its finished line to the notice once")
    f = code(body(cpp, "static void IvtFatalNote(const char* why"))
    check("fatalNoteWritten = true;" in f and f.count("NoticeFatal(injections > 0, refused[0] != 0);") == 1, "IvtFatalNote says it on the screen once, with stopped-or-never and where the note went")
    e = code(body(cpp, "static HRESULT __stdcall hkEndScene(IDirect3DDevice9* dev)"))
    check("DT_NOT_ON_WORKER(" in e and e.count("NoticeFrame(dev);") == 1 and e.count("{ NoticeFrame(dev); HdrEncode(dev); }") == 1 and e.count("HdrEncode(dev)") == 1,
          "the frame's end calls the notice once, right before the one HDR encode")
    allc = code(cpp)
    norm = re.sub(r"[ \t]+\n", "\n", allc)
    check(allc.count("NoticeWatchSecondCopy();") == 2 and allc.count("NoticeTellFirstCopy();") == 2 and "NoticeWatchSecondCopy();\n    if (dlssivClearedEv) SetEvent(dlssivClearedEv);" in norm,
          "the first copy makes the event before it lets a second copy on; the second sets it (a prototype and a call each)")
    check(cpp.rstrip("\n").endswith('#include "notice.h"'), "notice.h is the last line of DLSS-IV.cpp")
    p = code(body(hdr, "static void IvtParseCfg(")); nb = code(body(hdr, "static void IvtNoteBadCfg(const char* fmt, const char* k, const char* v)"))
    check("_stricmp(k, \"cpu_place\")" in p and "ivtCfgBadSeen++;" in nb and "notice" not in p.lower() and "ntc" not in p.lower() and "notice" not in nb.lower() and "ntc" not in nb.lower(),
          "neither the cfg's parser nor IvtNoteBadCfg names the notice (controls: each was found by a line of its own)")
    # THE NOTICE'S EYES (2026-10-11): the first build read hdrSceneDraws, HDR's own count, which stands at 0 with hdr=0, and
    # every proof with a stand-in passed while the owner's game showed nothing. Held here as text: what the frame's side
    # reads is the tracker's count of world matrices, and that count is made in hkSetVsConstF under the tracker's own
    # condition, which names no HDR state.
    step = code(body(nh, "static __declspec(noinline) void NoticeStep(IDirect3DDevice9* dev)"))
    check("NTC_NOW()" in step and "m4bWorldSerial" in step and "toldVerdict" in step and "frames" in step and "hdrSceneDraws" not in code(nh) and "hdrBbDraws" not in code(nh),
          "the notice reads the world from the tracker's world matrices, the armed flag and the frame counter, and no count of HDR's")
    sv = code(body(cpp, "static HRESULT __stdcall hkSetVsConstF(IDirect3DDevice9* dev, UINT start,"))
    k = sv.find("m4bWorldSerial++;")
    cond = sv[sv.rfind("\n    if (", 0, k):k] if k > 0 else ""
    check(sv.count("m4bWorldSerial++;") == 1 and cond.strip().startswith("if (!dead && toldVerdict && data && start == 0 && count >= 4)") and "hdr" not in cond.lower() and cond.count("if (") == 1,
          "that count is made in hkSetVsConstF once, under `!dead && toldVerdict && data && start == 0 && count >= 4` and nothing of HDR's")
    lc = code(body(hdr, "static void IvtLogCfgComplaints(const char* when)"))
    check(lc.count("NoticeCfgOverride();") == 1 and lc.index("NoticeCfgOverride();") < lc.index("if (ivtCfgBadSeen <= 0) return;"), "the cfg's complaints are scrubbed of the override before they are counted or said")

    print("2. every warning the source can write, against the table")
    cut = cut_rules(nh)
    check("IDirect3D" not in cut and "NoticeFromWarn" in cut and "NoticeCfgOverride" in cut, "the cut holds the rules and no Direct3D (control)")
    ok, log = build(cut)
    if not check(ok, "nr.exe builds from the cut"):
        print(log); return 1
    work = tempfile.mkdtemp(prefix="nrproof-")
    exe = os.path.join(HERE, "nr.exe")
    bad = part2(exe, work)
    for b in bad[:12]: print("        " + b)
    check(not bad, "every warning gives the notice the owner approved, or none: %d differences" % len(bad))

    print("3. the queue, the fade, the did-not-start texts, the cfg override, the two copies")
    outp = os.path.join(work, "rules.txt")
    r = subprocess.run([exe, "rules", outp], capture_output=True, text=True)
    v = dict(l.split("=", 1) for l in open(outp, encoding="latin-1").read().splitlines() if "=" in l) if r.returncode == 0 else {}
    check(v.get("done") == "1", "nr.exe rules ran to its end")
    check(v.get("once.queued") == "1" and v.get("once.afterShown") == "0", "a text is a notice once a session")
    check(v.get("again.whileWaiting") == "1" and v.get("again.later") == "1 pending=0", "the cfg's reply comes each time, and not twice while it waits")
    check(v.get("more.pendingBefore") == "1" and v.get("more.taken") == "5" and [v.get("more.%d" % i) for i in range(4)] == ["N%d|x|10" % i for i in range(4)]
          and v.get("more.4") == "DLSS-IV: and 2 more.|%s|10" % MORE_WARN and v.get("more.thenNothing") == "1 pending=0", "four in the order they came, then the rest as one line, then nothing waits")
    check(v.get("empty.queued") == "0" and v.get("drop.queued") == "0 pending=0", "an empty text is no notice; a dropped queue is empty")
    check([v.get("shown.%d" % t) for t in (0, 125, 250, 5000, 9600, 9800, 9999, 10000, 20000)] == ["0.0000", "0.5000", "1.0000", "1.0000", "1.0000", "0.5000", "0.0025", "0.0000", "0.0000"]
          and v.get("shown15.14800") == "0.5000", "the fade: in over a quarter second, whole, out over 0.4 s, gone at ten seconds (fifteen for a failed start)")
    check(v.get("fatal.start.0") == "DLSS-IV could not start, so the game runs without it.|%s|15" % MORE_FATAL
          and v.get("fatal.stopped.0") == "DLSS-IV stopped, so the game now runs without it.|%s|15" % MORE_FATAL
          and v.get("fatal.elsewhere.0") == "DLSS-IV could not start, so the game runs without it.|The reason is in DLSS-IV-DID-NOT-START.txt, in the TEMP folder|15", "the did-not-start note's three forms, fifteen seconds each")
    check(v.get("cfg.zero") == 'off 1 n 2 seen 2 | sharpnes = "0.5" is not a key this build knows | hdr = "x" is not a number', "notice=0 turns the notices off and leaves the list of unusable lines without it, the others in their order")
    check(v.get("cfg.other") == 'off 0 n 1 seen 3 | noticed = "0" is not a key this build knows' and v.get("cfg.none") == "off 0 n 0 seen 0", "notice=1 and notice=00 leave them on and are no complaint either; another key that begins the same is left alone")
    check(v.get("twin.before") == "0" and v.get("twin.after.0") == "%s|%s|10" % (TWO, MORE_WARN), "the second copy's word reaches the first copy's queue, and nothing before it")

    print("4. the controls")
    for what, old, new in (("the Fullscreen reason misspelt", '"exclusive fullscreen"', '"exclusive fullscrean"'),
                           ("\"Windows HDR is off\" no longer silent", 'if (strstr(line, "Windows HDR is off")) return;', ''),
                           ("the start-up check's line misspelt", '"DXVK could not end a state block recording"', '"DXVK could not end a state block recordinG"')):
        if not check(cut.count(old) == 1, "CONTROL (%s): its text stands once in the cut" % what): continue
        mexe = os.path.join(work, "nr_mut.exe")
        ok, log = build(cut.replace(old, new), mexe)
        check(ok and len(part2(mexe, work, quiet=True)) > 0, "CONTROL (%s): fails part 2, as it must" % what)
    # 4 (2026-10-28, XeSS X3): the DLSS rule widened to any upscaler's fall to FSR and to any missing file. DLSS's own causes
    # give what they gave, so what fails is XeSS's missing file shown as DLSS's notice, which the table holds to none.
    pairs = (('NtcStarts(line, "upscaler=-1 (automatic) is running FSR, not DLSS: ")', 'NtcStarts(line, "upscaler=-1 (automatic) is running FSR, not ")'),
             ('strstr(line, "nvngx_dlss.dll is not there")', 'strstr(line, "dll is not there")'))
    if check(all(cut.count(o) == 1 for o, _ in pairs), "CONTROL (a XeSS reason given DLSS's notice): its two texts stand once each in the cut"):
        spoiled = cut
        for o, n in pairs: spoiled = spoiled.replace(o, n)
        mexe = os.path.join(work, "nr_mut.exe")
        ok, log = build(spoiled, mexe)
        mb = part2(mexe, work, quiet=True) if ok else ["did not build"]
        check(ok and len(mb) > 0 and all(b.startswith("XeSS's reasons give") for b in mb),
              "CONTROL (a XeSS reason given DLSS's notice): fails part 2 on XeSS's reasons alone, as it must (%s)" % "; ".join(mb)[:160])
    print("FAIL: %d" % len(fails) if fails else "PASS")
    return 1 if fails else 0


if __name__ == "__main__":
    sys.exit(main())
