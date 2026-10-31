r"""THE PACING LAW AGAINST ITS ORACLE (2026-10-05): source/fg/fg_law.h, the C++ the rig runs at fg_test_law=1, against
fglaw_oracle.py, the law as the research round's critic wrote it, on the same sample streams, compared at every step.

    python equiv_fglaw.py             the proof: 0 differences wanted
    python equiv_fglaw.py control     the control: the oracle with one constant changed; the C++ must then FAIL at the comparison

What it does, in order:
  1. builds the streams: the owner's recorded F1C-5a costs (streams/5a-2026-10-05-free-us.txt) as they came, scaled to
     the floor's edge and scaled to a game slower than the floor; and seeded synthetic ones for what no recorded run holds
     (a run of hitches, levels in the free range, a hold band, fast and slow panels, counts at their caps);
  2. runs the oracle over each, and checks the law's own properties on its answers: never a period under the floor; a
     move never lands between the hold band and the free range but where the top put it;
  3. checks the streams themselves: every branch of the law is taken (a branch never taken is not proved), and every
     constant of the law, changed, changes the oracle's answers on them (a constant the streams cannot feel is not proved);
  4. writes samples, times and the oracle's answers to a file in %TEMP%, builds equiv_fglaw.cpp over fg_law.h with the
     mod's x86 toolchain (run_proof.py) and runs it: the C++ steps the same samples and compares the period, E, L, the
     regime and the hitch flag of every step, exactly.
SINCE 2026-10-17 (phase 5, the pacer's package) every stream runs twice: as the law of 2026-10-05 at its head of 1.06 (form
0, FgLawStep: every binding but the merged path), and as the free band's form at a head of 1.0 (form 1, FgLawStepForm: the
merged path); the file's head carries each stream's head and form (FGLAW02).
"""
import os, math, random, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from fglaw_oracle import LC, CONST, BRANCHES, loop

F120 = 16.916            # the floor of the owner's panel as the rig's rows carry it (two refreshes of 120 Hz times LOCK 1.015)


def need(c, what):
    if not c:
        print('EQUIV_FGLAW FAIL: ' + what)
        sys.exit(2)


def recorded():
    p = os.path.join(HERE, 'streams', '5a-2026-10-05-free-us.txt')
    v = []
    for ln in open(p, encoding='utf-8'):
        if ln.startswith('#'): continue
        v += [int(x) for x in ln.split()]
    need(len(v) == 16799, 'the recorded stream holds %d values, 16799 wanted' % len(v))
    return [x / 1000.0 if x > 0 else 0.0 for x in v]


def noise(rng, n, mean, sd, lo=0.3):
    return [max(lo, rng.gauss(mean, sd)) for _ in range(n)]


def levels(rng, plan, sd):
    """A cost that sits at each level of the plan for its count of frames, with gaussian noise."""
    out = []
    for lv, n in plan: out += noise(rng, n, lv, sd)
    return out


def streams():
    S = []
    def add(name, floor, hold, free, cost): S.append(dict(name=name, floor=floor, hold=hold, free=free, cost=cost, head=1.06, form=0))
    rec = recorded()
    srt = sorted(x for x in rec if x > 0); p95 = srt[int(len(srt) * 0.95)]
    add("5a's recorded costs, as they came", F120, 0.0, 20.0, rec)
    add("5a's recorded costs, a hold band to 17.4 ms", F120, 17.4, 20.0, rec)
    for k in (0.97, 1.00, 1.03):
        add("5a's costs scaled: the 95th percentile at %.2f of the floor" % k, F120, 0.0, 20.0, [x * (k * F120 / p95) for x in rec])
    add("5a's costs scaled: the 95th percentile at the floor, a hold band to 17.9 ms", F120, 17.9, 20.0, [x * (F120 / p95) for x in rec])
    add("5a's costs times 3.2: a game slower than the floor", F120, 0.0, 20.0, [x * 3.2 for x in rec])
    add("5a's costs times 3.2 on a 144 Hz panel", 14.097, 0.0, 16.5, [x * 3.2 for x in rec[:9000]])
    r = random.Random(1)
    c = noise(r, 6000, 6.0, 1.2)
    for i in range(300, 6000, 600): c[i] = 31.0 + r.random() * 12.0
    add('quiet, one long frame every 600', F120, 0.0, 20.0, c)
    r = random.Random(2)
    add('quiet, then 25 frames of 30 ms, then quiet', F120, 0.0, 20.0, noise(r, 900, 6.0, 1.2) + [30.0 + r.random() for _ in range(25)] + noise(r, 2400, 6.0, 1.2))
    r = random.Random(3)
    add('the door at its edge: seven hitches, one of 95 ms, twenty more', F120, 0.0, 20.0,
        noise(r, 400, 6.0, 1.0) + [30.0] * 7 + [95.0] + [30.0 + 0.1 * i for i in range(20)] + noise(r, 1500, 6.0, 1.0))
    r = random.Random(4)
    add('the door: eight hitches, 239 ordinary frames, a hitch, one more frame, a hitch', F120, 0.0, 20.0,
        noise(r, 400, 6.0, 1.0) + [30.0] * 8 + noise(r, 239, 6.0, 1.0) + [29.0] + noise(r, 1, 6.0, 1.0) + [28.0] + noise(r, 1200, 6.0, 1.0))
    r = random.Random(5)
    plan = [(6.0, 500)]
    for _ in range(40): plan.append((r.choice((6.0, 9.0, 15.5, 17.2, 18.4, 19.5, 21.0, 22.5, 24.0, 27.0, 30.0)), r.randint(250, 1100)))
    add('levels from 6 to 30 ms, no hold band', F120, 0.0, 20.0, levels(r, plan, 0.6))
    r = random.Random(6)
    plan = [(6.0, 400)]
    for _ in range(60): plan.append((r.choice((6.0, 15.2, 15.9, 16.2, 16.5, 16.75, 16.95, 17.2, 17.5, 17.9, 18.6, 20.5, 23.0)), r.randint(300, 1000)))
    add('levels round the floor, a hold band 2.0 ms wide', F120, F120 + 2.0, 20.0, levels(r, plan, 0.22))
    r = random.Random(7)
    plan = [(6.0, 400)]
    for _ in range(50): plan.append((r.choice((6.0, 15.4, 15.8, 16.0, 16.2, 16.4, 16.9, 17.6, 19.0, 21.0)), r.randint(300, 1000)))
    add('levels round the floor, a hold band to 17.4 ms', F120, 17.4, 20.0, levels(r, plan, 0.15))
    r = random.Random(8)
    plan = [(3.0, 600)]
    for _ in range(40): plan.append((r.choice((3.0, 5.0, 7.6, 8.2, 8.6, 9.3, 10.4, 11.5, 13.0)), r.randint(400, 1800)))
    add('a 240 Hz panel: floor 8.458 ms, free from 10.0', 8.458, 0.0, 10.0, levels(r, plan, 0.25))
    r = random.Random(9)
    plan = [(8.0, 400)]
    for _ in range(30): plan.append((r.choice((8.0, 14.0, 19.0, 20.6, 22.0, 26.0)), r.randint(250, 900)))
    add('the floor above the free range (LOCK 1.2 at 120 Hz: 20.0 ms, free asked from 19.0)', 20.0, 0.0, 19.0, levels(r, plan, 0.5))
    r = random.Random(10)
    def slow_panel(n): return [38.0 + r.random() * 4.0 if r.random() < 0.2 else max(0.5, r.gauss(8.0, 1.0)) for _ in range(n)]
    add('a 60 Hz panel (floor 33.833 ms), a light game with a fifth of its frames at 40 ms: no place over the floor', 33.833, 0.0, 20.0, slow_panel(1500))
    add('the same, a hold band to 34.8 ms', 33.833, 34.8, 20.0, slow_panel(1500))
    add('the same, free from 40.0 ms', 33.833, 0.0, 40.0, slow_panel(1500))
    r = random.Random(11)
    add('a light game, 8 percent of its frames at 23 to 25 ms: the top holds the rise', F120, 0.0, 20.0,
        [23.0 + r.random() * 2.0 if r.random() < 0.08 else max(0.5, r.gauss(3.0, 0.5)) for _ in range(3000)])
    add('a game at 45 to 60 ms: the 40 ms top', F120, 0.0, 20.0, [45.0 + r.random() * 15.0 for _ in range(900)] + noise(r, 1500, 6.0, 1.0))
    r = random.Random(12)
    add('a floor of 2.5 ms, free from 2.8: both counts at their caps, and a way down the 12 s memory decides', 2.5, 0.0, 2.8,
        levels(r, [(1.2, 2000), (2.62, 3000), (1.2, 7000), (1.6, 1500), (2.66, 2500), (1.3, 7000)], 0.05))
    r = random.Random(13)
    c = noise(r, 2500, 6.0, 1.5, lo=0.05)
    for i in range(0, 2500, 9): c[i] = 0.25
    for i in range(4, 2500, 31): c[i] = 0.1
    c[700] = 150.0; c[1400] = 6000.0; c[1401] = 0.0
    c += levels(r, [(18.0, 600), (6.0, 900)], 0.5)
    add('samples at the edges of what is taken, and a six second gap', F120, 0.0, 20.0, c)
    r = random.Random(14)
    add('slow from the first frame (the warm-up ends on a rise)', F120, 0.0, 20.0, levels(r, [(19.0, 500), (6.0, 1200)], 0.5))
    # (2026-10-17, the free band's form) a game that walks round the floor, and one in the free range with a spread of 3 percent
    r = random.Random(15)
    add('a walk round the floor, 15.8 to 17.8 ms and back, a spread of 3 percent', F120, 0.0, 20.0,
        [max(0.3, r.gauss(16.8 + math.sin(i / 2500.0), 0.504)) for i in range(15000)])
    r = random.Random(16)
    add('a game of 23.8 ms, a spread of 3 percent', F120, 0.0, 20.0, noise(r, 6000, 23.8, 0.714))
    # THE FREE BAND'S FORM (2026-10-17): every stream above once more, as the merged path runs the law (form 1, a head of 1.0)
    S0 = list(S)
    for s in S0: S.append(dict(s, name=s['name'] + ' (form 1, head 1.0)', head=1.0, form=1))
    # (2026-10-17, evening: AND ONCE MORE WITH THE FREE RANGE FROM THE FLOOR, the armed build's default since the owner's eye on leg 7)
    for s in S0: S.append(dict(s, name=s['name'] + ' (form 1, head 1.0, free from the floor)', head=1.0, form=1, free=0.0))
    return S


# ONE CHANGE A CONSTANT, small and to a side that binds (step 3: each must change the oracle's answers on the streams)
CHANGED = dict(hitch=1.42, door=9, open=239, fe=0.06, fl=0.03, ke_min=7, kl_min=6, up=7, down=13, tol_hold=0.25, step_hold=0.25,
               least_free=0.5, least_hold=0.3, band=0.04, band_min=0.45, top=17.0, top_max=39.0, fast=4500.0, slow=13000.0,
               fast_cap=1000, slow_cap=4000, warm=149, lo=0.3, hi=90.0, half=0.6,
               up_tol_pm=5, back_pm=1050)


def answers(S, k=None):
    out = []; hits = {}
    for s in S:
        law = LC(s['floor'], s['hold'], s['free'], s['head'], k, s['form'])
        out.append(loop(law, s['cost']))
        for n, v in law.hits.items(): hits[n] = hits.get(n, 0) + v
    return out, hits


def differ(a, b):
    """Steps on which two sets of answers differ in what the C++ compares (period, E, L, regime, hitch)."""
    return sum(1 for x, y in zip(a, b) for u, v in zip(x, y) if u[2:7] != v[2:7])


def main():
    control = len(sys.argv) > 1 and sys.argv[1] == 'control'
    need(set(CHANGED) == set(CONST), 'a constant of the oracle has no changed value here: %s' % sorted(set(CONST) ^ set(CHANGED)))
    S = streams()
    true, hits = answers(S)
    steps = sum(len(a) for a in true)
    print('  %d streams, %d steps' % (len(S), steps))
    # 2. THE LAW'S OWN PROPERTIES
    for s, a in zip(S, true):
        F = s['floor']; hm = max(s['hold'], F); fm = max(s['free'], hm); prev = F; moves = 0; under = 0; between = 0
        for c, t, P, E, L, reg, hitch, top in a:
            if P < F - 1e-9: under += 1
            if P != prev:
                moves += 1
                if hm + 1e-9 < P < fm - 1e-9 and abs(P - top) > 1e-9: between += 1
            prev = P
        at = sum(1 for x in a if x[5] == 0)
        print('    %-104s %6d steps, %3d moves, %5.1f percent at the floor, max %.3f ms' % (s['name'][:104], len(a), moves, 100.0 * at / len(a), max(x[2] for x in a)))
        need(under == 0, '%s: %d periods under the floor' % (s['name'], under))
        need(between == 0, '%s: %d moves to a period between the hold band and the free range that the top did not put there' % (s['name'], between))
    # the first stream is the run that started this: the law must not leave the floor on it (the research's own finding)
    need(all(x[2] == F120 for x in true[0]), "the law left the floor on 5a's recorded costs")
    need(all(S[j]['form'] == 1 and S[j]['cost'] is S[0]['cost'] and all(x[2] == F120 for x in true[j]) for j in (len(S) // 3, 2 * len(S) // 3)), "the free band's form left the floor on 5a's recorded costs")
    print("    on 5a's recorded costs the law never leaves the floor, in either form (the old law spent 5.6 percent of that run on it)")
    # 3. THE STREAMS: every branch, every constant
    print('  branches taken, in steps:')
    missing = [b for b in BRANCHES if not hits.get(b)]
    for b in BRANCHES: print('    %-62s %8d%s' % (b, hits.get(b, 0), '' if hits.get(b) else '   NEVER TAKEN'))
    need(not [h for h in hits if h not in BRANCHES], 'a branch the oracle names is not in its list: %s' % [h for h in hits if h not in BRANCHES])
    need(not missing, '%d branches never taken: %s' % (len(missing), missing))
    print('  constants, each changed alone: steps on which the oracle then answers otherwise')
    deaf = []
    for name in CONST:
        other, _ = answers(S, {name: CHANGED[name]})
        d = differ(true, other)
        print('    %-11s %-8s to %-8s %8d%s' % (name, CONST[name], CHANGED[name], d, '' if d else '   CHANGES NOTHING'))
        if not d: deaf.append(name)
    need(not deaf, 'the streams cannot feel %d constants: %s' % (len(deaf), deaf))
    # 4. THE C++
    if control:
        exp, _ = answers(S, {'down': CHANGED['down']})
        print('  CONTROL: the answers written are those of an oracle with down = %s (12 in the law); the C++ must differ' % CHANGED['down'])
    else: exp = true
    out = os.path.join(tempfile.gettempdir(), 'dlssiv_equiv_fglaw.%d' % os.getpid())
    os.makedirs(out, exist_ok=True)
    data = os.path.join(out, 'streams.bin')
    with open(data, 'wb') as fh:
        fh.write(b'FGLAW02\0' + struct.pack('<i', len(S)))
        for s, a in zip(S, exp):
            nm = s['name'].encode('ascii')
            fh.write(struct.pack('<i', len(nm)) + nm + struct.pack('<dddddi', s['floor'], s['hold'], s['free'], s['head'], float(s['form']), len(a)))
            fh.write(b''.join(struct.pack('<7d', x[0], x[1], x[2], x[3], x[4], float(x[5]), float(x[6])) for x in a))
    with open(os.path.join(out, 'equiv_fglaw_gen.h'), 'w', encoding='utf-8', newline='\n') as fh:
        fh.write('// GENERATED by equiv_fglaw.py: where this run\'s streams and the oracle\'s answers are\n')
        fh.write('static const char EQUIV_FGLAW_DATA[] = "%s";\nstatic const int EQUIV_FGLAW_STREAMS = %d;\nstatic const long long EQUIV_FGLAW_STEPS = %d;\n'
                 % (data.replace('\\', '/'), len(S), steps))
    sys.stdout.flush()
    r = subprocess.run([sys.executable, os.path.join(HERE, 'run_proof.py'), 'equiv_fglaw'] + (['control'] if control else []) + ['-I' + out], cwd=HERE)
    for f in ('streams.bin', 'equiv_fglaw_gen.h'):
        try: os.remove(os.path.join(out, f))
        except OSError: pass
    try: os.rmdir(out)
    except OSError: pass
    return r.returncode


sys.exit(main())
