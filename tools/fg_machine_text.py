"""fg_machine.h's text with its parts pasted back where it includes them: what a tool that reads the machine by path reads.

source/fg/fg_machine.h holds the vocabulary, the keys, the binding contract and the machine's head, and includes five parts
IN PLACE (an #include on a line of its own where the part's text stood): fg_machine_state.h at file scope, and inside
FgMachine's body fg_machine_render.h, fg_machine_worker.h, fg_machine_armed.h and fg_machine_placer.h. The compiler sees
one text in the order it always had; a tool that looks for a line, a function's body or an #if around a site must see
the same text, so it reads the machine through here.

    import fg_machine_text                      (tools/ on sys.path)
    fg_machine_text.read(path, encoding="utf-8", errors="strict", crlf=False)
        the file's text. For fg_machine.h, every part pasted in place of its #include line. crlf=False turns CRLF into LF,
        as open() in text mode does; crlf=True keeps the bytes' own line ends, as open(newline="") does.
    fg_machine_text.files(path)                 fg_machine.h's path and its parts' paths, in the order they are pasted
    fg_machine_text.holders(path, needle)       [(file's path, count)] of the files that hold `needle` (bytes or str)
    fg_machine_text.locate(path, line)          "file:line" of a line of read(path)'s text, in the file that holds it

A tree from before the split (fg_machine.h with no part included) reads as the one file it was. A tree that includes some
parts and not others, a part included twice, or an #include of a name that is not a part STOPS the tool (SystemExit):
the text it would read is not the text the compiler sees.
"""
import os
import re

PARTS = ("fg_machine_state.h", "fg_machine_render.h", "fg_machine_worker.h", "fg_machine_armed.h", "fg_machine_placer.h")
_INC = re.compile(rb'^[ \t]*#include "(fg_machine_[a-z_]+\.h)"[ \t]*\r?\n', re.M)


def _is_machine(path):
    return os.path.basename(path) == "fg_machine.h"


def _expand(path):
    """(bytes, segments): segments are (first line in the pasted text, file, first line in that file, lines), 1-based."""
    data = open(path, "rb").read()
    if not _is_machine(path):
        return data, [(1, path, 1, data.count(b"\n") + 1)]
    found = [m.group(1).decode("ascii") for m in _INC.finditer(data)]
    if not found:
        return data, [(1, path, 1, data.count(b"\n") + 1)]
    if sorted(found) != sorted(PARTS):
        raise SystemExit("fg_machine_text: %s includes the parts %s, where it must include each of %s once"
                         % (path, ", ".join(found), ", ".join(PARTS)))
    d = os.path.dirname(path)
    out, segs, at, line_out, line_in = [], [], 0, 1, 1
    for m in _INC.finditer(data):
        head = data[at:m.start()]
        n = head.count(b"\n")
        if n: segs.append((line_out, path, line_in, n))
        out.append(head); line_out += n; line_in += n + 1           # (the #include line itself is not in the text)
        part = os.path.join(d, m.group(1).decode("ascii"))
        body = open(part, "rb").read()
        if body and not body.endswith(b"\n"): body += b"\r\n" if b"\r\n" in body else b"\n"
        n = body.count(b"\n")
        segs.append((line_out, part, 1, n))
        out.append(body); line_out += n
        at = m.end()
    tail = data[at:]
    segs.append((line_out, path, line_in, tail.count(b"\n") + 1))
    out.append(tail)
    return b"".join(out), segs


def read(path, encoding="utf-8", errors="strict", crlf=False):
    t = _expand(path)[0].decode(encoding, errors)
    return t if crlf else t.replace("\r\n", "\n").replace("\r", "\n")


def files(path):
    data = open(path, "rb").read()
    if not _is_machine(path) or not _INC.search(data):
        return [path]
    _expand(path)                                                   # (the same refusals)
    return [path] + [os.path.join(os.path.dirname(path), m.group(1).decode("ascii")) for m in _INC.finditer(data)]


def holders(path, needle):
    nb = needle.encode("utf-8") if isinstance(needle, str) else needle
    out = []
    for f in files(path):
        n = open(f, "rb").read().count(nb)
        if n: out.append((f, n))
    return out


def locate(path, line):
    for first, f, first_in, n in _expand(path)[1]:
        if first <= line < first + n:
            return "%s:%d" % (os.path.basename(f), first_in + line - first)
    return "%s:%d" % (os.path.basename(path), line)
