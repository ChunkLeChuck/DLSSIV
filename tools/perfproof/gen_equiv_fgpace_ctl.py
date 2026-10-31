"""Writes equiv_fgpace's control 2: a COPY of source/fg/fg_machine.h with one constant of FgPace changed (since round 9's LW,
2026-10-27: PaceLaw's pad weight, 0.25 ms a quarter, made 0.26; until then A5's up-move dwell, 6 presents, made 7, which went
with A5's law), as fg_machine_ctl2.h in the folder named on the command line. The real header on disk is only read. The proof
built over the copy must FAIL at its comparison, which shows that what the comparison reads is the header's own pacer and not
something that happens to agree with the reference (2026-09-26, phase 1C, stage S4).

    python gen_equiv_fgpace_ctl.py <folder>
    python run_proof.py equiv_fgpace -DEQUIV_FGPACE_CONTROL=2 -I<folder>

It refuses (exit 1) when the line it changes is not in the header exactly once: a control whose target moved must be
pointed at the law again, never skipped. THE HEADER IS IN PARTS (fg_machine.h includes them in place, by name from its own
folder): the copy is fg_machine.h as fg_machine_ctl2.h with every part beside it, and the line is changed in the file
that holds it.
"""
import os, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.normpath(os.path.join(HERE, "..")))
import fg_machine_text   # (tools/fg_machine_text.py: fg_machine.h's parts)
SRC = os.path.normpath(os.path.join(HERE, "..", "..", "source", "fg", "fg_machine.h"))
OLD = b"else per += (double)InterlockedCompareExchange(&s.sh.padQms, 0, 0) * 0.25;"
NEW = b"else per += (double)InterlockedCompareExchange(&s.sh.padQms, 0, 0) * 0.26;"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    held = fg_machine_text.holders(SRC, OLD)
    n = sum(c for _, c in held)
    if n != 1:
        print("FAIL: %s and its parts hold %r %d times, not once; point control 2 at FgPace's join again" % (SRC, OLD.decode(), n))
        return 1
    target = held[0][0]
    data = open(target, "rb").read()
    out = data.replace(OLD, NEW)
    before, after = data.split(b"\n"), out.split(b"\n")
    changed = [i + 1 for i in range(len(before)) if before[i] != after[i]]
    if len(before) != len(after) or len(changed) != 1:
        print("FAIL: the copy differs from the header in %d lines, not 1" % len(changed))
        return 1
    folder = sys.argv[1]
    os.makedirs(folder, exist_ok=True)
    path = os.path.join(folder, "fg_machine_ctl2.h")
    for f in fg_machine_text.files(SRC):
        dst = path if f == SRC else os.path.join(folder, os.path.basename(f))
        if f == target:
            with open(dst, "wb") as fh:
                fh.write(out)
        else:
            shutil.copyfile(f, dst)
    # THE HEADER'S OWN INCLUDE (2026-10-05): fg_machine.h includes fg_law.h by name, from its own folder, so the copy needs it
    # beside it. Unchanged: the control changes one line of PaceLaw's joins and nothing of the law.
    shutil.copyfile(os.path.join(os.path.dirname(SRC), "fg_law.h"), os.path.join(folder, "fg_law.h"))
    # (2026-10-11: and fg_plan.h, the planned generation time and the room floor's law, which the merged placer reads)
    shutil.copyfile(os.path.join(os.path.dirname(SRC), "fg_plan.h"), os.path.join(folder, "fg_plan.h"))
    # (round 8, 2026-10-26: and fg_fixrule.h, the fixed mode's correction rule, which fg_machine_state.h includes by name)
    shutil.copyfile(os.path.join(os.path.dirname(SRC), "fg_fixrule.h"), os.path.join(folder, "fg_fixrule.h"))
    print("wrote %s with its parts beside it: %s with line %d changed, PaceLaw's pad weight 0.25 -> 0.26 ms (the only difference)"
          % (path, os.path.basename(target), changed[0]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
