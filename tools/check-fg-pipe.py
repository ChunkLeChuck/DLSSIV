r"""WHO MAY WRITE FRAME GENERATION'S MESSAGES TO THE PIPE (2026-10-06, phase 2 stage 11; the wire's B11, the machine's map
item 16).

The helper reads one pipe, and a message is whole only if one thread at a time writes it. The wire allows two writers of 'Q'
and 'G': THE DEVICE THREAD'S WORKER (the notes it runs and the observers it calls), and THE RENDER THREAD WITH THE RING
DRAINED (a build's head, a disarm's tail). Nothing else: not the hooked Present on the render thread, not the placer, not a
stamper, not the stop (which can run on any thread while the worker writes). This holds the rule from the source, before any
run (the product's count of its writers by thread, which a development line read, went in round 9's sweep):
(Since lever 2, 2026-10-10, the injection's observer writes 'Q' behind its body: it is the worker's, as it always was, and
the functions that write 'Q' are held to an exact list.)

  1. THE MACHINE (fg_machine.h). The functions whose bodies call B::WriteQ or B::WriteG, and every function that calls one
     of those, closed transitively over the file's call graph, are all in the list below, each with the thread it runs on.
     A note handed to the ring by its address is not a call: that IS the handoff to the worker.
  2. THE PRODUCT'S WAY IN (fg_product.h, fg_client.h, the transport, DLSS-IV.cpp). Of the machine's functions that reach a
     write, product code calls one, FgPM::BeforeRelease, from one function, FgpBeforeRelease; that is called from
     FgxBeforeRelease alone, and that from IvtBuild alone (the build, the ring drained).
  3. THE MESSAGES THEMSELVES. FgxWriteQ and FgxWriteG (fg_xport.h) are named in one body each, FgP::WriteQ and FgP::WriteG;
     no body outside the machine names WriteQ or WriteG; the seam PipeWrite is called by the three exchange functions of
     fg_xport.h; and IvtPipeWrite, the pipe itself, is called by the functions listed below and no other (the transport's
     own four, the product's seam, and a development build's sample at a build).
The scan is check-fg-export.py's, moved here (braces tracked, comments, literals and preprocessor lines skipped, FAILS when it loses its
place). IT FAILS WHEN IT IS BLIND: fewer writes found than the floor, or a function the product's shape must reach missing
from the reach. CONTROLS, every run, on copies in memory, each must FAIL: a WriteG put into SwapPresent; a raw IvtPipeWrite
of a 'Q' put into the product (FgpBeforeRelease); a WriteQ put into Stop; and the machine's quiesce called from the
product's stop (FgpStop).

    python tools\check-fg-pipe.py
"""
import importlib.util, os, re, sys
import fg_machine_text   # (fg_machine.h is in parts included in place: it is read with them pasted back, tools/fg_machine_text.py)

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# THE SCAN (braces tracked, comments, literals and preprocessor lines skipped, FAILS when it loses its place): check-fg-export.py's,
# moved here whole when round 8's deep strip retired that check with the frame generation rig's export
KEYWORDS = {"if", "for", "while", "switch", "catch", "sizeof", "return", "__declspec", "alignas", "decltype", "static_assert"}
SIG = re.compile(r"([A-Za-z_][\w:]*)\s*\((?:[^()]|\([^()]*\))*\)\s*(?:const\s*)?(?:noexcept\s*)?$")
IDENT = re.compile(r"[A-Za-z_]\w*")


class LostPlace(Exception):
    pass


def scan(text, watch):
    """(calls, mentions): calls are (pos, name, enclosing function) for every identifier followed by '(' inside a function body;
    mentions are (pos, name, enclosing) for every watched identifier anywhere. Raises LostPlace when the braces do not balance
    or a watched call sits outside every body and is not a declaration."""
    calls, mentions = [], []
    stack = []
    i, n = 0, len(text)
    head_start = 0
    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i); i = n if j < 0 else j; continue
        if text.startswith("/*", i):
            j = text.find("*/", i + 2); i = n if j < 0 else j + 2; continue
        if c == '"' or c == "'":
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == "\\" else 1
            i = j + 1; continue
        if c == "#" and text[:i].rsplit("\n", 1)[-1].strip() == "":
            j = text.find("\n", i)
            while j > 0 and text[j - 1] == "\\": j = text.find("\n", j + 1)
            i = n if j < 0 else j; head_start = i; continue
        if c == "{":
            head = re.sub(r"//[^\n]*|/\*.*?\*/", " ", text[head_start:i], flags=re.S).strip()
            m = SIG.search(head)
            ms = re.search(r"\b(struct|class)\s+(?:__declspec\([^)]*\)\s*)?(\w+)[^;]*$", head)
            if m and m.group(1).split("::")[-1] not in KEYWORDS and not re.search(r"[=]\s*$", head):
                outer = next((nm for k, nm in reversed(stack) if k == "type"), None)
                inner_fn = next((nm for k, nm in reversed(stack) if k == "fn"), None)
                name = m.group(1)
                if inner_fn is None and outer and "::" not in name: name = outer + "::" + name
                stack.append(("fn" if inner_fn is None else "block", name))
            elif ms and not m:
                stack.append(("type", ms.group(2)))
            else:
                stack.append(("block", None))
            i += 1; head_start = i; continue
        if c == "}":
            if not stack: raise LostPlace("a closing brace at line %d with nothing open" % (text.count("\n", 0, i) + 1))
            stack.pop()
            i += 1; head_start = i; continue
        if c == ";":
            i += 1; head_start = i; continue
        m = IDENT.match(text, i)
        if m:
            j = m.end(); name = m.group(0)
            fn = next((nm for kk, nm in reversed(stack) if kk == "fn"), None)
            if name in watch: mentions.append((i, name, fn))
            k = j
            while k < n and text[k] in " \t": k += 1
            if k < n and text[k] == "(":
                if fn is not None: calls.append((i, name, fn))
                elif name in watch:
                    # outside every body: only a declaration or a definition, its parameter list followed by ';' or '{'
                    depth, q = 0, k
                    while q < n:
                        if text[q] == "(": depth += 1
                        elif text[q] == ")":
                            depth -= 1
                            if depth == 0: break
                        q += 1
                    r = q + 1
                    while r < n and (text[r].isspace() or text.startswith("const", r) or text.startswith("noexcept", r)):
                        r += 5 if text.startswith("const", r) else 8 if text.startswith("noexcept", r) else 1
                    if r >= n or text[r] not in ";{":
                        raise LostPlace("%s( at line %d sits outside every function body and is neither a declaration nor a definition" % (name, text.count("\n", 0, i) + 1))
            i = j; continue
        i += 1
    if stack: raise LostPlace("%d brace(s) still open at the end of the file" % len(stack))
    return calls, mentions


MACHINE = os.path.join("source", "fg", "fg_machine.h")
PRODUCT = os.path.join("source", "fg", "fg_product.h")
CLIENT = os.path.join("source", "fg", "fg_client.h")
XPORT = os.path.join("source", "fg", "fg_xport.h")
TRANSPORT = os.path.join("source", "transport", "DlssIvTransport.h")
MOD = os.path.join("source", "DLSS-IV.cpp")
FILES = [MACHINE, PRODUCT, CLIENT, XPORT, TRANSPORT, MOD]

WRITES = {"WriteQ", "WriteG"}
# THE MACHINE'S FUNCTIONS THAT MAY REACH A WRITE, and the thread each runs on
WORKER = {
    "NoteHand", "NoteHudless", "NoteFire", "NoteCloseOpen",      # notes: the ring runs them on the worker
    "InjectObs", "Obs82", "ObsCheckpoint", "WorkerPresent",      # observers and the present: called by the worker
    "Fire", "FireN", "ClosePair",                                # called by the notes and the observers above
    "ShadowHand", "ShadowHudless", "ShadowCloseG", "ShadowWriteOwed",
    # lever 2 (2026-10-10): 'Q' behind the injection's body, and the close of one whose frame never reached a hand (called
    # by the worker's functions above, and by the two drained ones below, which the wire allows to write)
    "ShadowAfterInject", "ShadowCloseEarly",
    # the merged path (2026-10-10, phase 3, stage 7): its 'G' is written from the HUD-less note (ArmedG); its present, its
    # fires, its fire note and its order before an injection's body are the worker's (each can close a pair, and a
    # close on the worker writes); ArmedDrop is called by those and by the two drained ones below, where a close is OWED
    "ArmedG", "ArmedPresent", "ArmedFire", "ArmedFireN", "ArmedFireNote", "ArmedBeforeInject", "ArmedDrop",
    # stage 15 (2026-10-10): the present that is not made because its HUD-less picture could not be taken back closes the
    # pair open for it. Called by WorkerPresent alone, behind OnWorker()
    "ArmedHudDropped",
    # (2026-10-17, the list mended: it failed on the base since the cost merge) the ride's fire behind the injection's body, called by
    # InjectObs alone, on the worker; and the checkpoint observer of a build with the saves' tracker, installed in dtObs beside
    # ObsCheckpoint and run by the worker as every observer is
    "ArmedAfterInject", "ObsCheckpointAt",
}
DRAINED = {"BeforeRelease", "OnDisarm"}                          # the render thread (or the disarming one), the ring drained
ALLOWED = WORKER | DRAINED
MUST_REACH = {"NoteHand", "NoteHudless", "ShadowHand", "ShadowHudless", "ShadowCloseG", "ShadowWriteOwed", "BeforeRelease",
              "InjectObs", "ShadowAfterInject", "ShadowCloseEarly",   # (lever 2: the injection's observer writes 'Q' now)
              "ArmedG"}                                               # (stage 7: the merged path's 'G')
MIN_WRITES = 10         # the write sites of the machine; raised with them, never lowered without a DECISIONS entry (10 since round 9's strip, 2026-10-26: the shadow shape's own 'G' in ShadowHudless went with that shape; docs\INSTRUMENTS-FG.md, wave 5, is its entry)
# WHO WRITES 'Q' (lever 2, 2026-10-10): the rig's hand, the product's hand, and the injection's observer behind its body.
# Exactly these: every 'Q' must be closed by a 'G' before the next, and each of these three has its closers; a fourth
# writer would be a 'Q' nothing closes.
Q_WRITERS = {"NoteHand", "ShadowHand", "ShadowAfterInject"}
# THE PRODUCT'S WAY IN: (the calling function, the machine's function) for every reach function product code calls
WAY_IN = {("FgpBeforeRelease", "BeforeRelease")}
CALLERS = {"FgpBeforeRelease": {"FgxBeforeRelease"}, "FgxBeforeRelease": {"IvtBuild"}}
# THE MESSAGES
NAMED_IN = {"FgxWriteQ": "FgP::WriteQ", "FgxWriteG": "FgP::WriteG"}
SEAM_CALLERS = {XPORT: {"FgxExchange", "FgxWriteQ", "FgxWriteG"}}
PIPE_CALLERS = {CLIENT: {"FgxB::PipeWrite"},   # (FgxDevSample, a development build's sample at a build, until round 8's deep strip)
                TRANSPORT: {"IvtSendFrame", "IvtEnsureHost", "IvtSyncProbe", "IvtBuild"}}


def short(fn):
    return fn.split("::")[-1] if fn else fn


def line_of(text, pos):
    return text.count("\n", 0, pos) + 1


def qualified(text, pos):
    """The qualifier just before the name at pos ('FgPM' in FgPM::Stop), or None."""
    m = re.search(r"([A-Za-z_]\w*)\s*::\s*$", text[:pos])
    return m.group(1) if m else None


def check(texts):
    """(problems, facts) for the six files' texts, keyed by their relative paths."""
    bad = []; facts = {}
    watch = WRITES | set(NAMED_IN) | {"PipeWrite", "IvtPipeWrite"} | set(CALLERS)
    scans = {}
    for rel in FILES:
        try:
            scans[rel] = scan(texts[rel], watch)
        except cfe.LostPlace as e:
            return ["%s: the scan lost its place: %s" % (rel, e)], facts
    # 1. THE MACHINE
    calls, _ = scans[MACHINE]
    graph = {}; writers = {}; qwriters = set()
    for pos, name, fn in calls:
        graph.setdefault(short(fn), set()).add(name)
        if name in WRITES: writers.setdefault(short(fn), []).append(line_of(texts[MACHINE], pos))
        if name == "WriteQ": qwriters.add(short(fn))
    reach = set(writers)
    changed = True
    while changed:
        changed = False
        for fn, callees in graph.items():
            if fn not in reach and callees & reach: reach.add(fn); changed = True
    nw = sum(len(v) for v in writers.values())
    facts["writes"] = nw; facts["writers"] = sorted(writers); facts["reach"] = sorted(reach)
    if nw < MIN_WRITES: bad.append("fg_machine.h: only %d write(s) of 'Q' and 'G' were found where the floor is %d (the scan is blind)" % (nw, MIN_WRITES))
    for fn in sorted(MUST_REACH - reach): bad.append("fg_machine.h: %s does not reach a write, and the product's shape writes through it (the scan is blind)" % fn)
    facts["qwriters"] = sorted(qwriters)
    for fn in sorted(qwriters - Q_WRITERS): bad.append("fg_machine.h: %s writes 'Q', and only the hand and the injection's observer may (%s)" % (fn, ", ".join(sorted(Q_WRITERS))))
    for fn in sorted(Q_WRITERS - qwriters): bad.append("fg_machine.h: %s does not write 'Q' (the scan is blind)" % fn)
    for fn in sorted(reach - ALLOWED):
        via = "line %s" % ", ".join(str(n) for n in writers[fn]) if fn in writers else "through %s" % ", ".join(sorted(graph[fn] & reach))
        bad.append("fg_machine.h: %s reaches a write of 'Q' or 'G' (%s) and is not one of the worker's functions nor one the render thread runs with the ring drained" % (fn, via))
    # 2. THE PRODUCT'S WAY IN
    way = set(); callers = {k: set() for k in CALLERS}
    for rel in (PRODUCT, CLIENT, TRANSPORT, MOD):
        for pos, name, fn in scans[rel][0]:
            if name in reach and qualified(texts[rel], pos) == "FgPM":
                way.add((fn, name))
                if (fn, name) not in WAY_IN:
                    bad.append("%s: %s calls FgPM::%s (line %d), which reaches a write of 'Q' or 'G': the product's one way in is FgpBeforeRelease" % (rel, fn, name, line_of(texts[rel], pos)))
            if name in CALLERS and fn is not None:
                callers[name].add(fn)
                if fn not in CALLERS[name]:
                    bad.append("%s: %s is called from %s (line %d), not from %s alone" % (rel, name, fn, line_of(texts[rel], pos), ", ".join(sorted(CALLERS[name]))))
    for w in sorted(WAY_IN - way): bad.append("the product's way in, %s calling FgPM::%s, was not found (the scan is blind)" % w)
    for k in sorted(CALLERS):
        if not callers[k]: bad.append("%s is never called from a function body (the scan is blind)" % k)
    facts["way"] = sorted(way)
    # 3. THE MESSAGES
    named = {k: set() for k in NAMED_IN}; seam = 0; pipe = 0
    for rel in FILES:
        calls, mentions = scans[rel]
        for pos, name, fn in mentions:
            if fn is None: continue
            if name in NAMED_IN:
                named[name].add(fn)
                if fn != NAMED_IN[name]: bad.append("%s: %s is named in %s (line %d), not in %s alone" % (rel, name, fn, line_of(texts[rel], pos), NAMED_IN[name]))
            elif name in WRITES and rel != MACHINE:
                bad.append("%s: %s is named in %s (line %d): outside the machine nothing writes 'Q' or 'G'" % (rel, name, fn, line_of(texts[rel], pos)))
        for pos, name, fn in calls:
            if name == "PipeWrite":
                seam += 1
                if fn not in SEAM_CALLERS.get(rel, set()): bad.append("%s: the seam PipeWrite is called from %s (line %d), not from fg_xport.h's exchange functions" % (rel, fn, line_of(texts[rel], pos)))
            elif name == "IvtPipeWrite":
                pipe += 1
                if fn not in PIPE_CALLERS.get(rel, set()): bad.append("%s: IvtPipeWrite, the pipe itself, is called from %s (line %d), which is not one of its listed writers" % (rel, fn, line_of(texts[rel], pos)))
    for k in sorted(NAMED_IN):
        if not named[k]: bad.append("%s is named in no function body (the scan is blind)" % k)
    if seam < 3: bad.append("fg_xport.h: only %d call(s) of the seam PipeWrite were found (the scan is blind)" % seam)
    if pipe < 6: bad.append("only %d call(s) of IvtPipeWrite were found (the scan is blind)" % pipe)
    facts["seam"] = seam; facts["pipe"] = pipe
    return bad, facts


def plant(text, head, body):
    """text with body put first in the function whose definition matches head (a regular expression ending at its '{')."""
    m = re.search(head, text)
    return None if not m else text[:m.end()] + "\n" + body + text[m.end():]


def control(texts, rel, head, body, names):
    t = plant(texts[rel], head, body)
    if t is None: return False
    b, _ = check(dict(texts, **{rel: t}))
    return any(all(n in x for n in names) for x in b)


def main():
    texts = {rel: fg_machine_text.read(os.path.join(ROOT, rel), encoding="latin-1") for rel in FILES}   # (fg_machine.h with its parts pasted back)
    bad, f = check(texts)
    if f:
        print("  fg_machine.h: %d write(s) of 'Q' and 'G' in %d function(s) (%s); %d function(s) reach one, all the worker's or the drained render thread's (%s)" % (
            f.get("writes", 0), len(f.get("writers", [])), ", ".join(f.get("writers", [])), len(f.get("reach", [])), ", ".join(f.get("reach", []))))
        print("  the product's way in: %s; the seam's calls %d, the pipe's %d; files scanned %d" % (
            ", ".join("%s to FgPM::%s" % w for w in f.get("way", [])) or "NONE", f.get("seam", 0), f.get("pipe", 0), len(FILES)))
        print("  'Q' is written by: %s" % ", ".join(f.get("qwriters", [])))
    for b in bad: print("  " + b)
    c1 = control(texts, MACHINE, r"\bSwapPresent\s*\([^)]*\)\s*\{", "        B::WriteG(S().w.openP, 1u);", ["SwapPresent reaches a write"])
    c2 = control(texts, PRODUCT, r"\bstatic bool FgpBeforeRelease\s*\(\s*\)\s*\{", "    { FeedFgQ q = {}; q.tag = 'Q'; IvtPipeWrite(&q, sizeof(q)); }", ["IvtPipeWrite", "FgpBeforeRelease"])
    c3 = control(texts, MACHINE, r"\bstatic void Stop\s*\(const char\* why\)\s*\{", "        B::WriteQ(0, 0);", ["Stop reaches a write"])
    c4 = control(texts, PRODUCT, r"\bstatic void FgpStop\s*\([^)]*\)\s*\{", "    FgPM::BeforeRelease();", ["FgpStop calls FgPM::BeforeRelease"])
    print("  control 1, a WriteG put into SwapPresent: %s" % ("FAILS as it must" if c1 else "NOT SEEN, the check is blind"))
    print("  control 2, a raw IvtPipeWrite of a 'Q' put into the product: %s" % ("FAILS as it must" if c2 else "NOT SEEN, the check is blind"))
    print("  control 3, a WriteQ put into Stop: %s" % ("FAILS as it must" if c3 else "NOT SEEN, the check is blind"))
    print("  control 4, the machine's quiesce called from the product's stop: %s" % ("FAILS as it must" if c4 else "NOT SEEN, the check is blind"))
    c5 = control(texts, MACHINE, r"\bstatic void ShadowHudless\s*\([^)]*\)\s*\{", "        B::WriteQ(0, 0);", ["ShadowHudless writes 'Q'"])
    print("  control 5, a WriteQ put into the HUD-less note: %s" % ("FAILS as it must" if c5 else "NOT SEEN, the check is blind"))
    ok = not bad and c1 and c2 and c3 and c4 and c5
    print("PASS" if ok else "FAIL")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
