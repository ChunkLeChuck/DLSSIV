r"""THE MACHINE'S OWN RUN, REPLAYED THROUGH THE PACING LAW'S REFERENCE (2026-10-05).

equiv_fglaw.py proves source/fg/fg_law.h against the oracle on streams of samples. This proves the step between: that the
rig's pacer (fg_machine.h, PaceLaw) hands its law exactly what the present's record carries, so a run's rows, read from
the export a leg leaves, replay through the oracle with no difference. That replay is what tools/fg-pacing-read.py does
with a leg of the new law, and it is the check that the game ran the law it says it ran.

    python replay_pd8.py

It BUILDS and runs dt_fg_test's case PD8 (run_proof.py, every time: an exe from before an edit would replay the old
machine and pass) with DLSSIV_PD8_ROWS naming a file: 1,300 real frames through FgSwapPresent on the fake clock under the
new law (one long frame the law must call a hitch, then 25 slow frames, a rise, and the way back onto the floor), each
frame's present record written as the export writes its B row.
WHAT IT CAN SEE AND WHAT IT CANNOT (the batch's review, 2026-10-05). The frames are not whole microseconds, so a machine
that handed its law the UNCUT sample would write other rows than the replay computes: this run is refused if fewer than
1,000 of its samples have a fraction of a microsecond. It CANNOT tell the frame's entry from the pacer's own clock read
a moment later as the law's time: on the fake clock the original present takes no time, and in the game it takes 0 us
on 99.8 percent of frames. That the law's time is the entry is by reading (fg_machine.h, PaceLaw and PresWrite).
Beside it goes a log with the lines the reader takes the keys from, in fg_rig.h's own words (the rig went in round 9; the
words stay the reader's), and since round 9's LW the law every build runs: no hold band, the free range from the floor
(free_min 0), the free band's form at a head of 1.0 ("fg law form 1 head 1000"). The reader must say
the period, E, L and the hitch flag differ on 0 rows of at least 1,300, with at least one move and one hitch among them
(a replay of a run in which nothing happened shows nothing). CONTROL: one row's period changed by 137 us: the reader must
say exactly one row differs, and that it is not the pad.
"""
import os, re, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
READER = os.path.normpath(os.path.join(HERE, '..', 'fg-pacing-read.py'))


def fail(what):
    print('REPLAY FAIL: ' + what)
    sys.exit(1)


def read(export):
    r = subprocess.run([sys.executable, READER, export, '--hz', '120'], capture_output=True, text=True)
    txt = r.stdout + r.stderr
    m = re.search(r'THE REPLAY through the reference: (\d+) rows from the engage; the period differs on (\d+), E or L on (\d+), the hitch flag on (\d+)', txt)
    return txt, (tuple(int(g) for g in m.groups()) if m else None)


def main():
    out = os.path.join(tempfile.gettempdir(), 'dlssiv_replay_pd8.%d' % os.getpid())
    os.makedirs(out, exist_ok=True)
    export = os.path.join(out, '20261005-000000 PD8 presents.csv')
    log = os.path.join(out, '20261005-000000 PD8 DLSS-IV.log')
    env = dict(os.environ, DLSSIV_PD8_ROWS=export)
    r = subprocess.run([sys.executable, os.path.join(HERE, 'run_proof.py'), 'dt_fg_test', '2', 'PD8'], cwd=HERE, env=env, capture_output=True, text=True)
    if r.returncode != 0 or 'PASS PD8' not in r.stdout: fail('dt_fg_test PD8 did not pass (exit %d): %s' % (r.returncode, r.stdout[-600:]))
    if not os.path.exists(export): fail('PD8 wrote no rows (DLSSIV_PD8_ROWS was set to %s)' % export)
    rows = open(export, encoding='utf-8').read().splitlines()
    b = [ln for ln in rows if ln.startswith('B,')]
    moved = sum(1 for ln in b if int(ln.split(',')[5]) & 8); hitch = sum(1 for ln in b if int(ln.split(',')[5]) & 256)
    figs = sum(1 for ln in b if int(ln.split(',')[11]) > 0)
    print('  PD8 wrote %d rows: %d moves of the period, %d hitches, the law\'s figures on %d' % (len(b), moved, hitch, figs))
    if len(b) < 1300 or moved < 3 or hitch < 2 or figs < 1200: fail('the rows do not hold a run worth replaying')
    # the sample in ticks, as Pace takes it: this row's entry less the release before it (the original present takes no time
    # on the fake clock); a fraction of a microsecond is what the cut removes
    cols = [ln.split(',') for ln in b]
    frac = sum(1 for i in range(1, len(cols)) if (int(cols[i][2]) - int(cols[i - 1][4])) % 10)
    print('  samples with a fraction of a microsecond (what the cut to whole microseconds removes): %d of %d' % (frac, len(cols) - 1))
    if frac < 1000: fail('the frames are whole microseconds: the replay could not see the sample\'s cut')
    # the reader's own scan clock row (it needs a refresh rate; --hz gives it) and the keys' lines, in fg_rig.h's words
    open(export, 'w', encoding='utf-8', newline='\n').write('\n'.join(rows) + '\n')
    open(log, 'w', encoding='utf-8', newline='\n').write(
        '  cfg: fg_test_mode=0 (NORMAL) fg_test_shape=0 (Z) fg_test_abba=0 fg_test_abba_pace=0 fg_test_kfix_us=-1 fg_test_gen_us=1500 fg_test_hold_us=5000 fg_test_pace=1 fg_test_lock=1015 fg_test_spacing=0\n'
        '  cfg: fg_test_law=1 fg_test_hold_max_us=0 fg_test_free_min_us=0 fg_test_nshift_free_us=0 fg_test_period_us=0 fg_test_period_alt_ms=0 fg_test_plant=0 fg_test_plant_ms=31 fg_test_plant_every=600 fg_test_plant_run=1\n'
        '  fg law form 1 head 1000\n'
        '  fg rig: armed: shape Z, mode NORMAL, images 3 (x), hdr 0, abba 1, abba_pace 0, idle 0 (FGIDLE 0), early C1, spacing measured, kfix readiness, gen 1500 us, hold 5000 us, pace FgPace, lock 1.015, refresh 120/1, C 59.11 fps\n')
    txt, got = read(export)
    if not got: fail('the reader printed no replay line: ' + txt[-800:])
    print('  the reader: %d rows from the engage; the period differs on %d, E or L on %d, the hitch flag on %d' % got)
    if 'the floor is taken from the rows' in txt: fail('the reader did not take the floor from the log\'s LOCK and refresh rate')
    if got[0] < 1300 or got[1:] != (0, 0, 0): fail('the rig\'s rows do not replay through the law\'s reference')
    # every row stepped, in one engage (the reader steps each engage and says what it could not step)
    m = re.search(r'stepped (\d+) of the (\d+) rows, in (\d+) engage\(s\)', txt)
    if not m or m.group(1) != m.group(2) or int(m.group(1)) != len(b) or m.group(3) != '1' or 'NOT STEPPED' in txt: fail('the reader did not step every row of the one engage: ' + (m.group(0) if m else 'no coverage line'))
    # CONTROL: one row's period changed
    k = next(i for i, ln in enumerate(rows) if ln.startswith('B,') and int(ln.split(',')[1]) == int(b[900].split(',')[1]))
    c = rows[k].split(','); c[10] = str(int(c[10]) + 137); bad = list(rows); bad[k] = ','.join(c)
    open(export, 'w', encoding='utf-8', newline='\n').write('\n'.join(bad) + '\n')
    txt, got2 = read(export)
    print('  control, one row\'s period 137 us off: %s' % ('the period differs on %d row(s)' % got2[1] if got2 else 'no replay line'))
    if not got2 or got2[1] != 1 or 'THE REST ARE NOT EXPLAINED' not in txt: fail('the control: a wrong row was not found, or was called the pad')
    for p in (export, log):
        try: os.remove(p)
        except OSError: pass
    try: os.rmdir(out)
    except OSError: pass
    print('REPLAY PASS')
    return 0


sys.exit(main())
