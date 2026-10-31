r"""INTEL XeSS'S TEXTS AT THE HANDSHAKE, RUN OVER EVERY CASE (2026-10-28, XeSS X3 and X8; docs/reports/xess-plan-2026-10-17,
PLAN.md X3 and X8). source/DLSS-IV.cpp's end holds, between its XESS TEXTS START and END lines, what the client says when the
helper's caps say XeSS cannot run: IvtXessWhy (the reason), IvtXessFix (what to do instead) and IvtXessHandshake (which of
the two arms fires: upscaler=2 stops the session with IvtFail, automatic that fell to FSR writes IvtWarn). No Intel card and
no Linux machine is at this desk, so the texts are held here against every case, with IvtFail, IvtWarn, ivtCfg and ivtWine
stood in by the bench and the caps bits from transport/feed_ipc.h itself:

  - upscaler=2 on Windows and under Wine or Proton (X8), for each of the helper's three reasons and for none of them: the
    refusal, byte for byte, as the table below has it; the session stops (false) and nothing is warned;
  - automatic (upscaler=-1) on an Intel card that fell to FSR: the warning, byte for byte, the session goes on;
  - nothing at all where XeSS runs, where it was not probed, in transport-test mode (mode=1), for upscaler=0 and 1, and in
    automatic when FSR did not load either (IvtEnsureHost's own arm says that one);
  - every text: plain ASCII, no em dash, no we, us or our, no "log=1", under the 512 bytes the refusal's buffer holds;
  - THE WIRING, as text: IvtEnsureHost calls IvtXessHandshake once, after the upscaler=1 refusal and before automatic's
    arms, and stops on false; its prototype stands at the top of DLSS-IV.cpp; the ack sets ivtBackendId from
    FEED_ACK_BACKEND_XESS right after ivtBackendIsFsr;
  - TWO CONTROLS: the cut with the Wine line taken out of IvtXessFix, and with the NOFILE test taken out of IvtXessWhy, must
    each FAIL (a bench that could not fail would hold nothing).
  - (fg18-fix, F6, the release audit of 2026-10-28) AUTOMATIC'S FALL FROM XeSS TO FSR AT A BUILD (IvtXessFell: said once, only in
    automatic where the handshake said XeSS runs and a refused ack names FSR) and THE THIRD REFUSED BUILD'S STOP (IvtBuildRefusedWhy:
    upscaler=2's ends with IvtXessFix's own remedy, every other as before), each against its table; their wiring in IvtBuild as
    text; TWO MORE CONTROLS, the handshake's note taken out and the stop's remedy reworded, each must fail its own check.

    python tools\test-xess-texts\gen.py        exit code 0 = every check passed and both controls failed as they must
"""
import os, re, subprocess, sys, tempfile, shutil

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
CPP = os.path.join(ROOT, "source", "DLSS-IV.cpp")
HDR = os.path.join(ROOT, "source", "transport", "DlssIvTransport.h")
OUT = os.path.join(tempfile.gettempdir(), "dlssiv-test-xess-texts-%d" % os.getpid())
START = "// XESS TEXTS START"
END = "// XESS TEXTS END"

# THE APPROVED TEXTS (the report of XeSS X3, 2026-10-28, quotes each in full)
NOFILE = "Intel's DLSSIV\\libxess.dll is not there. It ships with this mod, so copy the DLSSIV folder over again"
UNSUP = "Intel's library says this graphics card or driver cannot run XeSS"
RT_WIN = ("libxess.dll would not load. It needs the Microsoft Visual C++ runtime 14.40 or later, which Microsoft's latest"
          " Visual C++ Redistributable for x64 installs")
RT_WINE = "libxess.dll would not load. It needs the Microsoft Visual C++ runtime 14.40 or later"
OTHER = "Intel's library could not start XeSS on this graphics card (the \"host:\" lines in DLSS-IV.log say why)"
FIX_WIN = ". Set upscaler=-1 and the mod picks for this card"
FIX_WINE = ". Under Proton the upscaler host runs on vkd3d-proton; if XeSS will not start there, upscaler=-1 runs FSR"
REFUSE = "upscaler=2 asks for XeSS and "
AUTO = "upscaler=-1 (automatic) is running FSR, not XeSS: "

# feed_ipc.h's bits, read from the header below and checked against these
BITS = {"FEED_CAP_FSR": 8, "FEED_CAP_XESS_PROBED": 16384, "FEED_CAP_XESS": 32768, "FEED_CAP_XESS_NOFILE": 65536,
        "FEED_CAP_XESS_UNSUPPORTED": 131072, "FEED_CAP_XESS_NORUNTIME": 262144}
FSR, P, X, NF, US, RT = 8, 16384, 32768, 65536, 131072, 262144


def cases(fsr):
    """(name, backend, mode, caps, wine, expected return, expected IvtFail text or None, expected IvtWarn text or None)"""
    c = []
    for wine in (0, 1):
        fix = FIX_WINE if wine else FIX_WIN
        w = " under Wine" if wine else " on Windows"
        c += [("upscaler=2, the file missing" + w, 2, 2, P | NF | fsr, wine, 0, REFUSE + NOFILE, None),
              ("upscaler=2, the card or driver refused" + w, 2, 2, P | US | fsr, wine, 0, REFUSE + UNSUP + fix, None),
              ("upscaler=2, no runtime" + w, 2, 2, P | RT | fsr, wine, 0, REFUSE + (RT_WINE if wine else RT_WIN) + fix, None),
              ("upscaler=2, no reason given" + w, 2, 2, P | fsr, wine, 0, REFUSE + OTHER + fix, None),
              ("upscaler=2, the file missing and the card refused: the file first" + w, 2, 2, P | NF | US | fsr, wine, 0, REFUSE + NOFILE, None),
              ("upscaler=2, XeSS runs" + w, 2, 2, P | X | fsr, wine, 1, None, None),
              ("upscaler=2, not probed (the helper's contract: it probes on 2)" + w, 2, 2, fsr, wine, 1, None, None),
              ("upscaler=2 in transport-test mode" + w, 2, 1, P | NF | fsr, wine, 1, None, None),
              ("upscaler=0 with XeSS's bits" + w, 0, 2, P | NF | fsr, wine, 1, None, None),
              ("upscaler=1 with XeSS's bits" + w, 1, 2, P | US | fsr, wine, 1, None, None),
              ("automatic, XeSS runs" + w, -1, 2, P | X | fsr, wine, 1, None, None),
              ("automatic, XeSS not probed (not an Intel card)" + w, -1, 2, fsr, wine, 1, None, None),
              ("automatic in transport-test mode" + w, -1, 1, P | US | fsr, wine, 1, None, None)]
        if fsr:
            c += [("automatic fell to FSR, the file missing" + w, -1, 2, P | NF | fsr, wine, 1, None, AUTO + NOFILE),
                  ("automatic fell to FSR, the card or driver refused" + w, -1, 2, P | US | fsr, wine, 1, None, AUTO + UNSUP),
                  ("automatic fell to FSR, no runtime" + w, -1, 2, P | RT | fsr, wine, 1, None, AUTO + (RT_WINE if wine else RT_WIN)),
                  ("automatic fell to FSR, no reason given" + w, -1, 2, P | fsr, wine, 1, None, AUTO + OTHER)]
        else:
            c += [("automatic, XeSS and FSR both missing (IvtEnsureHost's own arm says it)" + w, -1, 2, P | NF, wine, 1, None, None)]
    return c


ALL = cases(FSR) + cases(0)

BENCH_HEAD = r"""
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <cstdlib>
#include "%s"
#include "%s"
struct { int mode; int backend; } ivtCfg;
static bool ivtWine = false;
static int ivtXessFell = 0;   // (fg18-fix, F6: DLSS-IV.cpp's top static, stood in)
#define IVT_MODE ivtCfg.mode   // the development shape of the macro (DLSS-IV.cpp); a release is the constant 2, and the cases of mode 1 are the point here
static char gFail[2048]; static int nFail = 0;
static char gWarn[2048]; static int nWarn = 0;
static void IvtFail(const char* why) { strcpy_s(gFail, why); nFail++; }
static void IvtWarn(const char* fmt, ...) { va_list ap; va_start(ap, fmt); vsnprintf(gWarn, sizeof(gWarn), fmt, ap); va_end(ap); nWarn++; }
"""

BENCH_MAIN = r"""
struct Case { int backend, mode; unsigned long long caps; int wine; };
static const Case kCases[] = { %s };
int main(int argc, char** argv)
{
    FILE* f = nullptr; fopen_s(&f, argv[1], "wb");
    for (const Case& c : kCases)
    {
        ivtCfg.backend = c.backend; ivtCfg.mode = c.mode; ivtWine = c.wine != 0;
        nFail = nWarn = 0; gFail[0] = gWarn[0] = 0;
        const bool r = IvtXessHandshake(c.caps);
        fprintf(f, "%%d\t%%d\t%%d\t%%s\t%%s\n", r ? 1 : 0, nFail, nWarn, nFail ? gFail : "-", nWarn ? gWarn : "-");
    }
    fclose(f);
    // (fg18-fix, F6) THE FALL AT A BUILD: a session's handshake, then refused acks in order; one line a session: the warnings written
    FILE* g = nullptr; fopen_s(&g, argv[2], "wb");
    struct Fall { int backend; unsigned long long caps; int wine; unsigned acks[3]; int nAcks; };
    static const Fall kFall[] = { %s };
    for (const Fall& c : kFall)
    {
        ivtCfg.backend = c.backend; ivtCfg.mode = 2; ivtWine = c.wine != 0; ivtXessFell = 0;
        nFail = nWarn = 0; gFail[0] = gWarn[0] = 0;
        IvtXessHandshake(c.caps);
        const int w0 = nWarn; nWarn = 0; gWarn[0] = 0;
        for (int i = 0; i < c.nAcks; i++) IvtXessFell(c.acks[i]);
        fprintf(g, "%%d\t%%d\t%%s\n", w0, nWarn, nWarn ? gWarn : "-");
    }
    // (fg18-fix, F6) THE THIRD REFUSED BUILD'S STOP: one line each, and IvtXessFix's own words beside it (the two must agree)
    struct Why { int backend; unsigned flags; int wine; };
    static const Why kWhy[] = { %s };
    for (const Why& c : kWhy)
    {
        ivtCfg.backend = c.backend; ivtCfg.mode = 2; ivtWine = c.wine != 0;
        char why[384]; IvtBuildRefusedWhy(why, sizeof(why), c.flags);
        fprintf(g, "%%s\t%%s\n", why, IvtXessFix(0, ivtWine));
    }
    fclose(g);
    return 0;
}
"""
# (fg18-fix, F6) THE FALL AT A BUILD: (name, backend, caps at the handshake, wine, refused acks' flags, warnings at the handshake, the build's warning)
ACK_FSR, ACK_XESS, ACK_SRU = 4, 64, 2
FALL = [("automatic on Intel, XeSS at the handshake, a refused ack naming FSR, then another: said once", -1, P | X | FSR, 0, [ACK_FSR, ACK_FSR], 0, AUTO + OTHER),
        ("the same under Wine", -1, P | X | FSR, 1, [ACK_FSR], 0, AUTO + OTHER),
        ("automatic on another card (no XeSS bit), a refused FSR build: not a fall", -1, FSR, 0, [ACK_FSR], 0, None),
        ("automatic, XeSS at the handshake, a refused ack naming XeSS (no fall): nothing", -1, P | X | FSR, 0, [ACK_XESS], 0, None),
        ("upscaler=2, XeSS at the handshake, a refused ack naming XeSS: nothing (upscaler=2 never falls)", 2, P | X | FSR, 0, [ACK_XESS, ACK_XESS], 0, None),
        ("upscaler=1, a refused FSR build: nothing", 1, FSR, 0, [ACK_FSR], 0, None),
        ("automatic that fell at the handshake (XeSS probed, not able), then a refused FSR build: the handshake's line only", -1, P | NF | FSR, 0, [ACK_FSR], 1, None)]
BASE_WHY = ("three host builds were refused by the %s back end%s. The \"host:\" lines in DLSS-IV.log name the reason, which is normally a back end"
            " library this GPU refused or a shared texture the driver would not create")
WHY = [("upscaler=2, XeSS refused three times, Windows", 2, ACK_XESS, 0, BASE_WHY % ("XeSS", "") + FIX_WIN),
       ("upscaler=2, XeSS refused three times, under Proton", 2, ACK_XESS, 1, BASE_WHY % ("XeSS", "") + FIX_WINE),
       ("automatic, XeSS refused (no fall: not automatic's words)", -1, ACK_XESS, 0, BASE_WHY % ("XeSS", "") + "."),
       ("FSR refused three times", 1, ACK_FSR, 0, BASE_WHY % ("FSR", "") + "."),
       ("automatic after the fall, FSR refused three times", -1, ACK_FSR, 0, BASE_WHY % ("FSR", "") + "."),
       ("DLSS with no mode for the output", 0, ACK_SRU, 0, BASE_WHY % ("DLSS", " because NGX could not report a DLSS mode for this output size") + "."),
       ("DLSS refused", -1, 0, 0, BASE_WHY % ("DLSS", "") + ".")]


def cut():
    t = open(CPP, encoding="utf-8").read()
    a = t.find(START); b = t.find(END)
    if a < 0 or b < 0 or b < a or t.count(START) != 1 or t.count(END) != 1:
        raise SystemExit("FAIL: DLSS-IV.cpp has no single XESS TEXTS START / END pair in that order")
    body = t[a:b]
    for sig in ("static const char* IvtXessWhy(unsigned long long caps, bool wine)", "static const char* IvtXessFix(unsigned long long caps, bool wine)",
                "static bool IvtXessHandshake(unsigned long long caps)", "static void IvtXessFell(unsigned flags)",
                "static void IvtBuildRefusedWhy(char* why, size_t cap, unsigned flags)"):
        if sig not in body: raise SystemExit("FAIL: the cut does not hold %s" % sig)
    return body


def build_run(name, body):
    d = os.path.join(OUT, name); os.makedirs(d, exist_ok=True)
    src = os.path.join(d, "b.cpp")
    inc = [os.path.join(ROOT, "source", "transport", n).replace("\\", "/") for n in ("feed_ipc.h", "dlssiv_layout.h")]
    lits = ", ".join("{ %d, %d, %dull, %d }" % (b, m, caps, w) for _, b, m, caps, w, _, _, _ in ALL)
    fall = ", ".join("{ %d, %dull, %d, { %s }, %d }" % (b, caps, w, ", ".join("%du" % a for a in (acks + [0, 0, 0])[:3]), len(acks)) for _, b, caps, w, acks, _, _ in FALL)
    why = ", ".join("{ %d, %du, %d }" % (b, fl, w) for _, b, fl, w, _ in WHY)
    open(src, "w", encoding="utf-8").write(BENCH_HEAD % tuple(inc) + body + BENCH_MAIN % (lits, fall, why))
    vsw = os.path.expandvars(r"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe")
    ip = subprocess.check_output([vsw, "-latest", "-products", "*", "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64", "-property", "installationPath"]).decode().strip()
    vcvars = os.path.join(ip, r"VC\Auxiliary\Build\vcvars32.bat")
    r = subprocess.run('call "%s" >nul 2>&1 && cl /nologo /O2 /EHsc /W3 /std:c++17 b.cpp /Fe:b.exe' % vcvars, shell=True, capture_output=True, text=True, cwd=d)
    if r.returncode != 0:
        print(r.stdout[-2000:]); raise SystemExit("FAIL: the bench did not compile (%s)" % name)
    outp = os.path.join(d, "out.txt"); out2 = os.path.join(d, "out2.txt")
    g = subprocess.run([os.path.join(d, "b.exe"), outp, out2], capture_output=True, text=True, cwd=d)
    if g.returncode != 0: raise SystemExit("FAIL: the bench did not run (%s)" % name)
    rows = [l.split("\t") for l in open(outp, "rb").read().decode("latin-1").split("\n") if l]
    rows2 = [l.split("\t") for l in open(out2, "rb").read().decode("latin-1").split("\n") if l]
    return rows, rows2


def judge2(rows2):
    """(fg18-fix, F6) The fall at a build and the third refusal's stop, against FALL and WHY (empty: all of it held)."""
    bad = []
    if len(rows2) != len(FALL) + len(WHY): return ["the bench answered %d of the fall's and the stop's %d cases" % (len(rows2), len(FALL) + len(WHY))]
    for (name, _, _, _, _, w0, warn), r in zip(FALL, rows2[:len(FALL)]):
        got = (int(r[0]), None if r[1] == "0" else r[2])
        if r[1] not in ("0", "1"): bad.append("%s: the build's warning written %s times (once at most)" % (name, r[1]))
        if got != (w0, warn): bad.append("%s: got %r, the table %r" % (name, got, (w0, warn)))
    for (name, b, _, _, want), r in zip(WHY, rows2[len(FALL):]):
        if r[0] != want: bad.append("%s: got %r, the table %r" % (name, r[0], want))
        if b == 2 and not r[0].endswith(r[1]): bad.append("%s: the stop's remedy is not IvtXessFix's own words (%r)" % (name, r[1]))
    return bad


def judge(rows, quiet):
    """The differences from the table, as lines (empty: every case holds)."""
    bad = []
    if len(rows) != len(ALL): return ["the bench answered %d cases of %d" % (len(rows), len(ALL))]
    for (name, _, _, _, _, ret, fail, warn), row in zip(ALL, rows):
        got = (int(row[0]), row[3] if row[1] != "0" else None, row[4] if row[2] != "0" else None)
        if row[1] not in ("0", "1") or row[2] not in ("0", "1"): bad.append("%s: IvtFail %s times, IvtWarn %s times" % (name, row[1], row[2]))
        if got != (ret, fail, warn): bad.append("%s: got %r, the table %r" % (name, got, (ret, fail, warn)))
    return bad


def language(texts):
    bad = []
    for t in texts:
        if any(ord(ch) > 126 or ord(ch) < 32 for ch in t): bad.append("not plain ASCII: %r" % t)
        if re.search(r"\b(we|us|our)\b", t, re.I): bad.append("we, us or our: %r" % t)
        if "log=1" in t: bad.append("names log=1: %r" % t)
        if " - " in t or "--" in t: bad.append("a dash where a sentence should break: %r" % t)
        if len(t.encode()) >= 511: bad.append("%d bytes, past what the refusal's buffer holds: %r" % (len(t.encode()), t))
    return bad


def wiring():
    """The calls and the ack, as text, each inside the function it is about."""
    out = []
    cpp = open(CPP, encoding="utf-8").read(); hdr = open(HDR, encoding="utf-8").read()
    def body(text, head):
        a = text.index(head); return text[a:text.index("\n}\n", a)]
    def code(s): return "\n".join(l.split("//")[0] for l in s.splitlines())
    eh = code(body(hdr, "static bool IvtEnsureHost()"))
    k = eh.find("if (!IvtXessHandshake(ack.caps)) return false;")
    one = eh.count("IvtXessHandshake(") == 1 and k > 0
    after = one and eh.rfind('IvtFail("upscaler=1 asks for FSR', 0, k) > 0
    before = one and eh.find("if (ivtCfg.backend < 0 && IVT_MODE != 1", k) > k and eh.find("if (ivtCfg.backend < 0", 0, k) < 0
    out.append((one and after and before, "IvtEnsureHost calls IvtXessHandshake once and stops on false, after the upscaler=1 refusal and before automatic's arms"))
    top = cpp[:cpp.index('#include "transport/DlssIvTransport.h"')] if '#include "transport/DlssIvTransport.h"' in cpp else ""
    out.append((top.count("static bool IvtXessHandshake(unsigned long long caps);") == 1, "its prototype stands at the top of DLSS-IV.cpp, before the transport's include"))
    # (fg18-fix, F6) IvtBuild's refusal: IvtXessFell once, ahead of the count of three, and the stop's words from IvtBuildRefusedWhy once, inside it
    ib = code(body(hdr, "static bool IvtBuild(IDirect3DDevice9* dev)"))
    kf = ib.find("IvtXessFell(ack.flags);"); k3 = ib.find("if (++ivtBuildFails >= 3)"); kw = ib.find("IvtBuildRefusedWhy(why, sizeof(why), ack.flags);")
    out.append((ib.count("IvtXessFell(") == 1 and ib.count("IvtBuildRefusedWhy(") == 1 and 0 < kf < k3 < kw and ib.rfind("IvtFail(why);") > kw,
                "IvtBuild's refusal calls IvtXessFell once ahead of the count of three, and the stop's words come from IvtBuildRefusedWhy, then IvtFail"))
    out.append((top.count("static void IvtXessFell(unsigned flags);") == 1 and top.count("static void IvtBuildRefusedWhy(char* why, size_t cap, unsigned flags);") == 1
                and top.count("DLSSIV_FGDATA static int ivtXessFell;") == 1, "their prototypes and the fall's static (in .data$zfg) stand at the top of DLSS-IV.cpp"))
    stale = cpp.index('// ---- THE STALE "HDR IS NOT ON" WARNING')
    out.append((cpp.index("static void CfgSayOff()\n{") < cpp.index(START) < cpp.index(END) < stale and cpp.rstrip("\n").endswith('#include "notice.h"'),
                "the texts stand at DLSS-IV.cpp's end, above the stale HDR warning's section (test-warn-takeback cuts from it to notice.h, the last line)"))
    bld = code(body(hdr, "static bool IvtBuild(IDirect3DDevice9* dev)"))
    a = bld.find("ivtBackendIsFsr = (ack.flags & FEED_ACK_BACKEND_FSR) != 0;")
    b = bld.find("ivtBackendId = (ack.flags & FEED_ACK_BACKEND_XESS) ? 2 : ivtBackendIsFsr ? 1 : 0;")
    out.append((a > 0 and b > a and len(re.findall(r"\bivtBackendId\s*=(?!=)", bld)) == 1 and len(re.findall(r"\bivtBackendId\s*=(?!=)", hdr + cpp)) == 1,"the ack sets ivtBackendId from FEED_ACK_BACKEND_XESS once, right after ivtBackendIsFsr"))
    ipc = open(os.path.join(ROOT, "source", "transport", "feed_ipc.h"), encoding="utf-8").read()
    got = {n: int(m.group(1)) for n in BITS for m in [re.search(r"#define\s+%s\s+(\d+)ull" % n, ipc)] if m}
    out.append((got == BITS, "feed_ipc.h's bits are the ones this table is written for: %s" % ", ".join("%s %s" % (n, got.get(n)) for n in BITS)))
    return out


def main():
    os.makedirs(OUT, exist_ok=True)
    fails = 0
    def check(ok, what):
        nonlocal fails
        print("  %s  %s" % ("ok  " if ok else "FAIL", what))
        if not ok: fails += 1
        return ok
    try:
        print("1. the wiring")
        for ok, what in wiring(): check(ok, what)
        print("2. every case against the table")
        body = cut()
        rows, rows2 = build_run("real", body)
        bad = judge(rows, False)
        for b in bad[:12]: print("        " + b)
        check(not bad, "%d cases (upscaler=2 and automatic, Windows and Wine, every reason, and where nothing is said): %d differences" % (len(ALL), len(bad)))
        texts = sorted(set(t for r in rows for t in (r[3], r[4]) if t != "-"))
        lb = language(texts)
        for b in lb: print("        " + b)
        check(not lb and len(texts) == 12, "the %d texts it can write are plain ASCII, short enough, with no we, us, our, dash or log=1 (12: seven refusals, five warnings)" % len(texts))
        print("2b. (fg18-fix, F6) automatic's fall from XeSS to FSR at a build, and the third refused build's stop")
        b2 = judge2(rows2)
        for b in b2[:12]: print("        " + b)
        check(not b2, "%d sessions of the fall and %d stops against the table (the fall said once, only in automatic where the handshake said XeSS runs; upscaler=2's stop ends with IvtXessFix's own remedy): %d differences" % (len(FALL), len(WHY), len(b2)))
        t2 = sorted(set([r[2] for r in rows2[:len(FALL)] if r[1] != "0"] + [r[0] for r in rows2[len(FALL):]]))
        l2 = language(t2)
        for b in l2: print("        " + b)
        check(not l2, "the %d texts of the fall and the stop are plain ASCII, short enough, with no we, us, our, dash or log=1" % len(t2))
        print("3. the controls")
        for what, old, new in (("the Wine line taken out of IvtXessFix", "    if (wine) return \". Under Proton", "    if (false) return \". Under Proton"),
                               ("the missing file's test taken out of IvtXessWhy", "    if (caps & FEED_CAP_XESS_NOFILE)\n        return \"Intel's ", "    if (false)\n        return \"Intel's ")):
            if not check(body.count(old) == 1, "CONTROL (%s): its text stands once in the cut" % what): continue
            mb = judge(build_run("control", body.replace(old, new))[0], True)
            check(len(mb) > 0, "CONTROL (%s): fails, as it must (%d case(s))" % (what, len(mb)))
        # (fg18-fix, F6: the fall's two controls, each aimed at its own check)
        for what, old, new in (("the handshake's note that XeSS runs taken out", "    if (caps & FEED_CAP_XESS) ivtXessFell |= 1;", "    if (false) ivtXessFell |= 1;"),
                               ("the stop's Windows remedy reworded", "                  : \". Set upscaler=-1 and the mod picks for this card\");", "                  : \". Set upscaler=-1 and the mod picks for your card\");")):
            if not check(body.count(old) == 1, "CONTROL (%s): its text stands once in the cut" % what): continue
            mb = judge2(build_run("control2", body.replace(old, new))[1])
            check(len(mb) > 0, "CONTROL (%s): fails, as it must (%d case(s))" % (what, len(mb)))
        print("PASS" if not fails else "FAIL: %d" % fails)
        return 1 if fails else 0
    finally:
        shutil.rmtree(OUT, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
