# synth-sim.py: the phase 1B synthesis, 2026-09-24. The classifier spec (classifier-spec.md) as Python, run on D3's own
# physics model (d3-sim.py, imported unchanged) with the judges' grafts, against D3's runs, sweeps and stress cases and
# against every attack the three judges found. Four variants:
#   V0 = D3 as designed (fixed lag 3 or 2, rule 2 QUANTISED)                      : the baseline, must reproduce D3
#   V1 = the spec: rule 2 UNCERTAIN plus a latch, D3's fixed lag (3 armed, 2 not) kept as the anchor,
#        the memory from vsync-on windows below the ceiling only, R2 and S as diagnostics
#   V2 = V1 plus S on the QUANTISED side (R >= 0.50 and S >= 0.90 may stand in for R >= 0.80)
#   V3 = V1 with D2's anchor, the latest end at or before the release plus 0.5 ms (J1 and J2 graft; REFUTED here)
# A hard error is QUANTISED where the screen follows presents, or FREE where it quantises (d3-sim.py's hard_error).
# Run from wl-1b:  python synth/synth-sim.py      (never python -)
#
# THIS COPY (tools/flipcls, 2026-09-24, frame generation phase 1B, instrument I-B2): the phase's reference model, copied into
# the tree unchanged except in two places, so the classifier's C code can be replayed against it (the spec's checklist item 6).
#   1. It loads d3-sim.py (also copied unchanged, with d3-stress.py) from its own folder, not from the scratchpad's parent.
#   2. --dump <file> writes every V2 window the run makes as the waiter would see it, frame by frame, for tools/flipcls/run.py:
#      the frames' stamps as integer ticks of a 10 MHz clock (the QPC's rate on Windows 10 and later), the window's present
#      block as the float the C code is handed, and the model's verdict and display memory after it. In --dump mode the model
#      classifies the SAME rounded stamps and float block, so both sides see identical inputs; how many verdicts that rounding
#      changed against the unrounded run is printed, and should be none. Only V2 runs in --dump mode, plus a V2 copy of
#      sections 5 and 6 (section 5 as one continuous session, the latch and the memory carried across its two parts, as the
#      C statics carry them). Without --dump the script prints exactly what the scratchpad's did.
import importlib.util, os, math, zlib, sys
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__))
os.chdir(HERE)
spec = importlib.util.spec_from_file_location('d3sim', 'd3-sim.py'); d3 = importlib.util.module_from_spec(spec); spec.loader.exec_module(d3)
DUMP = sys.argv[sys.argv.index('--dump') + 1] if '--dump' in sys.argv else None
QT = 1e4                                                                     # dump ticks per ms: a 10 MHz clock
DF = open(DUMP, 'w') if DUMP else None
DSTAT = dict(cells=0, windows=0, rounding_changed=0, hard=0, membad=0)
C = d3.C; LAT_WIN = d3.LAT_WIN
TAB = [math.cos(2 * math.pi * (k + 0.5) / 32.0) for k in range(32)]      # the spec's 32-entry table
def tsin(k): return TAB[(k - 8) & 31]                                      # sin(x) = cos(x - pi/2): 8 bins

def resultant(cnt, n):
    c = sum(cnt[k] * TAB[k] for k in range(32)); s = sum(cnt[k] * tsin(k) for k in range(32))
    c2 = sum(cnt[k] * (2 * TAB[k] * TAB[k] - 1) for k in range(32)); s2 = sum(cnt[k] * 2 * tsin(k) * TAB[k] for k in range(32))
    return math.hypot(c, s) / n, math.hypot(c2, s2) / n

def windows2(sim, p, seed, drop=0.02):
    rng = np.random.default_rng(seed + 7)
    b = sim['b']; blk = sim['blk']; marker = p.get('marker', True)
    E = sim['tEnd'] if marker else sim['tIn']
    lag = 3 if p.get('threaded', 1) else 2
    out = []; cur = dict(b=[], iv=[], A=[], AL=[], blk=[]); prev_ok = None
    for x in range(200, len(b) - 1):
        if rng.random() < drop:
            prev_ok = None; continue
        cur['b'].append(b[x])
        if prev_ok == x - 1: cur['iv'].append(b[x] - b[x - 1])
        cur['A'].append(b[x] - E[x - lag])                                   # V0: the fixed lag
        cand = [E[j] for j in range(max(0, x - 8), x + 1) if E[j] <= b[x] + 0.5]
        cur['AL'].append(b[x] - max(cand) if cand else 1e9)                  # V1: the latest end at or before b + 0.5 ms
        cur['blk'].append(blk[x - 1]); prev_ok = x
        if len(cur['b']) == LAT_WIN:
            out.append(cur); cur = dict(b=[], iv=[], A=[], AL=[], blk=[])
    return out

def feats(w, T, latest):
    iv = np.asarray(w['iv']); b = np.asarray(w['b']); A = np.asarray(w['AL'] if latest else w['A'])
    n = len(iv); mean_iv = float(np.mean(iv)); med = float(np.median(iv))
    q25, q75 = np.percentile(iv, [25, 75]); iqr = float(q75 - q25)
    bins = np.floor(np.mod((b - b[0]) / T, 1.0) * 32.0).astype(int) & 31
    R, R2 = resultant(np.bincount(bins, minlength=32), len(b))
    st = np.diff(b)                                                            # every adjacent accepted pair, as D1 defines R_d
    sb = np.floor(np.mod(st / T, 1.0) * 32.0).astype(int) & 31
    Rd, _ = resultant(np.bincount(sb, minlength=32), len(st))
    S = R / math.sqrt(Rd) if Rd > 1e-6 else 0.0
    k = np.rint(iv / T); r = iv / T - k; on = (np.abs(r) <= C['EPS_G']) & (k >= 1); G = float(np.mean(on))
    if on.any():
        ks, cnt = np.unique(k[on], return_counts=True); order = np.argsort(-cnt)
        kmode = int(ks[order[0]]); kfrac = float(cnt[order[0]]) / n; mix2 = float(cnt[order[1]]) / n if len(cnt) > 1 else 0.0
    else:
        kmode, kfrac, mix2 = 0, 0.0, 0.0
    A = A[A < 1e8]
    Amed = float(np.median(A)) if len(A) else 1e9
    p10, p80 = (np.percentile(A, [10, 80]) if len(A) else (0, 1e9)); SA = float(p80 - p10)
    return dict(n=n, mean=mean_iv, med=med, iqr=iqr, R=R, R2=R2, Rd=Rd, S=S, G=G, kmode=kmode, kfrac=kfrac, mix2=mix2,
                Amed=Amed, SA=SA, hitch=float(np.mean(iv > C['HITCH_X'] * med)), rate=1000.0 / mean_iv)

def classify2(w, T, vs, F_cands, marker, block, variant, latch):
    """Returns (verdict, why, ft, pin, memok). pin: a latch candidate. memok: this window may feed the display memory."""
    if not T: return 'UNCERTAIN', 'the refresh is unknown', {}, False, False
    if len(w['iv']) < C['MIN_N']: return 'UNCERTAIN', 'too few', {}, False, False
    latest = variant == 3
    ft = feats(w, T, latest); ft['block'] = block
    mean_iv, med, iqr, R, G, S = ft['mean'], ft['med'], ft['iqr'], ft['R'], ft['G'], ft['S']
    if ft['rate'] < C['SLOW_FPS']: return 'UNCERTAIN', 'below 32 fps', ft, False, False
    if ft['hitch'] > C['HITCH_FRAC']: return 'UNCERTAIN', 'hitches', ft, False, False
    ceiling = mean_iv <= C['CEIL'] * T
    periodic = ft['kfrac'] >= C['PERIOD_K'] and ft['mix2'] < C['MIX']
    pinned = iqr <= C['PIN_IQR'] * med
    fmatch = None
    for F in F_cands:
        P = 1000.0 / F
        if abs(med - P) <= max(C['LIM_TOL'] * P, 0.25) and iqr <= 0.10 * P: fmatch = F
    lockQ = R >= C['R_HI'] or (variant == 2 and R >= 0.50 and S >= 0.90)
    if not ceiling and lockQ and G >= C['G_HI'] and ft['mix2'] >= C['MIX']:
        return 'QUANTISED', 'a phase-locked mix of whole refreshes', ft, False, (variant == 0 or vs)
    if (not vs) and abs(mean_iv - T) <= 0.015 * T and R >= C['R_HI'] and fmatch is None:
        if block >= C['BLOCK_PIN'] * T:
            if variant == 0: return 'QUANTISED', 'pinned at T, vsync off: the driver syncs', ft, False, True
            return 'UNCERTAIN', 'pinned at the refresh with vsync off: a sync the mod did not ask for', ft, True, False
        return 'UNCERTAIN', 'pinned at T, vsync off, no wait inside Present', ft, False, False
    if fmatch is not None: return 'LIMITED', 'known cap', ft, False, False
    if not vs:
        if variant >= 1 and latch: return 'UNCERTAIN', 'vsync off, a sync the mod did not ask for was seen', ft, False, False
        if mean_iv <= 0.97 * T and not pinned: return 'FREE', 'above the refresh and unsynced', ft, False, variant == 0
        return 'UNCERTAIN', 'vsync off below the refresh', ft, False, False
    if ceiling: return 'UNCERTAIN', 'ceiling', ft, False, False
    if variant >= 1 and R < C['R_LO'] and ft['R2'] >= C['R_HI']: return 'UNCERTAIN', 'locked to half the refresh period read', ft, False, False
    blocked =block >= C['BLOCK_FREE'] * mean_iv
    anchored = ft['Amed'] <= d3.a_lim(T, marker) or (blocked and not pinned and ft['Amed'] <= C['A_LATE'] * T)
    if R <= C['R_LO'] and anchored and ft['SA'] <= d3.s_lim(T, marker) and (marker or blocked):
        return 'FREE', 'steady short anchor, off the grid', ft, False, True
    return 'UNCERTAIN', 'other', ft, False, False

def run2(p, nw, seed, variant, T_read=None, _nodump=False):
    sim = d3.simulate(p, nw * LAT_WIN + 400, seed)
    T = T_read if T_read is not None else 1000.0 / p['hz']
    out = []; latch = False; pinrun = 0
    for w in windows2(sim, p, seed)[:nw]:
        block = float(np.mean(w['blk']))
        F = [p['limiter_known']] if p.get('limiter_known') else []
        v, why, ft, pin, memok = classify2(w, T, p['vs'], F, p.get('marker', True), block, variant, latch)
        pinrun = pinrun + 1 if pin else 0
        if pinrun >= 2: latch = True
        out.append((v, why, ft, memok))
    return out

def memory(res):
    """The display memory at the end, and the worst it held: two most recent informative windows agree, else none."""
    last = []; mem = None; held = set()
    for v, why, ft, memok in res:
        if v in ('FREE', 'QUANTISED') and memok:
            last = (last + [v])[-2:]
            if len(last) == 2: mem = last[0] if last[0] == last[1] else None
        if mem: held.add(mem)
    return mem, held

VCODE = {'FREE': 1, 'QUANTISED': 2, 'LIMITED': 3, 'UNCERTAIN': 4}
def run2_dump(p, nw, seed, T_read=None, truth=None, state=None, cont=False, tag=''):
    """V2 on stamps rounded to 10 MHz ticks and a float32 block, written to the dump as the waiter would see them.
    state carries the latch, its run and the memory's slots across a continued session (cont=True)."""
    sim = d3.simulate(p, nw * LAT_WIN + 400, seed)
    T = T_read if T_read is not None else 1000.0 / p['hz']
    q = {k: np.rint(sim[k] * QT).astype(np.int64) for k in ('b', 'tIn', 'tEnd')}
    qs = dict(b=q['b'] / QT, tIn=q['tIn'] / QT, tEnd=q['tEnd'] / QT, blk=sim['blk'])
    rng = np.random.default_rng(seed + 7)                                    # windows2's own draw, frame for frame
    marker = p.get('marker', True); lag = 3 if p.get('threaded', 1) else 2
    E = qs['tEnd'] if marker else qs['tIn']; b = qs['b']
    wins = []; cur = dict(b=[], iv=[], A=[], AL=[], blk=[], xs=[]); prev_ok = None; acc = set(); last_x = None
    for x in range(200, len(b) - 1):
        if rng.random() < 0.02:
            prev_ok = None; continue
        cur['b'].append(b[x]); cur['xs'].append(x); acc.add(x)
        if prev_ok == x - 1: cur['iv'].append(b[x] - b[x - 1])
        cur['A'].append(b[x] - E[x - lag]); cur['AL'].append(0.0)
        cur['blk'].append(sim['blk'][x - 1]); prev_ok = x
        if len(cur['b']) == LAT_WIN:
            wins.append(cur); cur = dict(b=[], iv=[], A=[], AL=[], blk=[], xs=[])
            if len(wins) == nw: last_x = x; break
    if state is None: state = dict(latch=False, pinrun=0, last=[], mem=None)
    if T and abs(1000.0 / T - round(1000.0 / T)) > 1e-9: raise SystemExit('a refresh read that is not a whole number of Hz: %r' % T)
    num = int(round(1000.0 / T)) if T else 0
    F = [p['limiter_known']] if p.get('limiter_known') else []
    DF.write('C %s %d %d %d %d %d %d\n' % (tag or 'cell', num, 1 if p['vs'] else 0, 1 if marker else 0, lag, F[0] if F else 0, 1 if cont else 0))
    ends = {w['xs'][-1]: i for i, w in enumerate(wins)}
    out = []
    for x in range(190, (last_x if last_x is not None else len(b) - 2) + 1):
        DF.write('F %d %d %d %d %d\n' % (x, q['tIn'][x], q['tEnd'][x], q['b'][x], 1 if x in acc else 0))
        if x in ends:
            w = wins[ends[x]]
            block = float(np.float32(np.mean(w['blk'])))
            v, why, ft, pin, memok = classify2(w, T, p['vs'], F, marker, block, 2, state['latch'])
            state['pinrun'] = state['pinrun'] + 1 if pin else 0
            if state['pinrun'] >= 2: state['latch'] = True
            if v in ('FREE', 'QUANTISED') and memok:
                state['last'] = (state['last'] + [v])[-2:]
                if len(state['last']) == 2: state['mem'] = state['last'][0] if state['last'][0] == state['last'][1] else None
            DF.write('W %r %d %d\n' % (block, VCODE[v], VCODE.get(state['mem'], 0) if state['mem'] else 0))
            out.append((v, why, ft, memok))
            DSTAT['windows'] += 1
            if truth and d3.hard_error(truth, v): DSTAT['hard'] += 1
            if truth and ((state['mem'] == 'QUANTISED' and truth == 'follow') or (state['mem'] == 'FREE' and truth == 'grid')): DSTAT['membad'] += 1
    DF.write('E\n'); DSTAT['cells'] += 1
    if not cont and T_read != 0:                                             # the rounding's own effect, against the unrounded run
        ref = run2(p, nw, seed, 2, T_read, _nodump=True)
        DSTAT['rounding_changed'] += sum(1 for r, o in zip(ref, out) if r[0] != o[0])
    return out, state

TOT = {0: [0, 0], 1: [0, 0], 2: [0, 0], 3: [0, 0]}          # windows, hard errors
MEMBAD = {0: 0, 1: 0, 2: 0, 3: 0}
def cell(p, truth, nw=30, seed=1, T_read=None, variants=(0, 1, 2, 3)):
    s = []
    if DUMP: variants = (2,)                                                 # --dump: V2 only, on the rounded stamps
    for vv in variants:
        res = run2_dump(p, nw, seed, T_read, truth)[0] if DUMP else run2(p, nw, seed, vv, T_read)
        c = {}; h = 0
        for v, _, _, _ in res:
            c[v[0]] = c.get(v[0], 0) + 1
            if d3.hard_error(truth, v): h += 1
        mem, held = memory(res)
        bad = ('QUANTISED' in held and truth == 'follow') or ('FREE' in held and truth == 'grid')
        TOT[vv][0] += len(res); TOT[vv][1] += h; MEMBAD[vv] += 1 if bad else 0
        s.append('%-14s%s%s' % (' '.join('%s%d' % (k, c[k]) for k in 'FQLU' if k in c), (' H%d' % h) if h else '', ' MEM!' if bad else ''))
    return ' | '.join(s)

def sd(*a): return zlib.crc32(repr(a).encode()) % 90000 + 1000

print('Columns: V0 (D3 as designed) | V1 (the spec) | V2 (V1 plus S gating QUANTISED) | V3 (V1 with the latest-end anchor). H = hard-error windows; MEM! = the display memory held a wrong class at some point.\n')
print('1. D3\'s runs and sceptic cases (40 windows each)')
for i, (name, exp, truth, p) in enumerate(d3.RUNS):
    print('%-48s %-6s %s' % (name, truth, cell(p, truth, 40, 1000 + i, p.get('T_read'))))

print('\n2. Judges\' attacks')
print('2a. balanced load, fixed 120 Hz, vsync on (J1 R1, J2 6.1), marker on and off')
for g, c in ((10.0, 9.0), (10.0, 10.0), (10.0, 11.0), (12.0, 12.0), (14.0, 13.0), (16.0, 15.0), (16.0, 16.0), (19.0, 19.0), (5.0, 8.2), (5.0, 7.5)):
    w = min(0.45 * g, 4.5)
    for mk in (True, False):
        kw = dict(disp='fixed', dxvk='fifo', vs=True, g=g, cr=c, cw=w)
        if not mk: kw.update(limiter=('ff', 120), limiter_known=120, marker=False)
        print('   g %-5.1f cr %-5.1f marker %-5s %s' % (g, c, mk, cell(d3.run(**kw), 'grid', 40, sd('bal', g, c, mk))))
print('2b. balanced load at 144 and 240 Hz')
for hz in (144.0, 240.0):
    T = 1000.0 / hz
    for mk in (True, False):
        kw = dict(hz=hz, disp='fixed', dxvk='fifo', vs=True, g=1.2 * T, cr=1.2 * T, cw=0.45 * T, g_sd=0.06 * T, cr_sd=0.05 * T, cw_sd=0.04 * T)
        if not mk: kw.update(marker=False)
        print('   %3.0f Hz marker %-5s %s' % (hz, mk, cell(d3.run(**kw), 'grid', 40, sd('bal2', hz, mk))))
print('2c. G-Sync plus driver vsync, game vsync off, above the refresh (J1 R6, J3 R-D3-1): VRR must never be QUANTISED')
print('   VRR   %s' % cell(d3.run(disp='vrr', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5), 'follow', 40, 555))
print('   fixed %s' % cell(d3.run(disp='fixed', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5), 'grid', 40, 556))
print('2d. a refresh read wrong by a non-harmonic ratio (J1 probe 3, J3 R-D1-1), fixed, GPU 10 ms')
for hz, rd in ((120.0, 144.0), (144.0, 120.0), (165.0, 144.0), (60.0, 144.0), (120.0, 60.0)):
    for mk in (True, False):
        p = d3.run(hz=hz, disp='fixed', dxvk='fifo', vs=True, g=10.0, marker=mk)
        print('   true %3.0f read %3.0f marker %-5s %s' % (hz, rd, mk, cell(p, 'grid', 30, sd('wr', hz, rd, mk), T_read=1000.0 / rd)))
print('2e. VRR, steady GPU at 2T and 3T, a known cap that does not bind, no marker (J2 6.2, J3 R-D2-1)')
for k in (2, 3):
    for noise in (0.02, 0.01, 0.005):
        g = k * 1000.0 / 120.0
        p = d3.run(disp='vrr', dxvk='fifo', vs=True, g=g - 0.05, g_sd=noise * g, g_white=noise * g * 0.5, limiter=('ff', 120), limiter_known=120, marker=False)
        print('   k %d noise %-5.3f %s' % (k, noise, cell(p, 'follow', 40, sd('2t', k, noise))))
print('2f. a cap inside Present the mod does not know (DXVK-style), VRR, vsync on, marker on and off (J2 6.3)')
for cap, g in ((60, 8.0), (60, 12.0), (60, 16.0), (60, 16.5), (40, 20.0), (40, 24.5)):
    for mk in (True, False):
        p = d3.run(disp='vrr', dxvk='fifo', vs=True, g=g, cr=min(5.0, 0.5 * g), cw=min(4.0, 0.4 * g), limiter=('dxvk', cap), marker=mk)
        print('   cap %d g %-5.1f marker %-5s %s' % (cap, g, mk, cell(p, 'follow', 30, sd('cap', cap, g, mk))))
print('2g. the FG band on a fixed panel, GPU-bound (J2 6.4): coverage, marker on and off')
for g in (16.0, 16.4, 16.67, 17.0, 17.5, 18.5, 20.0):
    for mk in (True, False):
        kw = dict(disp='fixed', dxvk='fifo', vs=True, g=g, marker=mk)
        print('   g %-5.2f marker %-5s %s' % (g, mk, cell(d3.run(**kw), 'grid', 40, sd('band', g, mk))))
print('2h. the same band on VRR')
for g in (16.0, 16.67, 17.5, 20.0):
    for mk in (True, False):
        print('   g %-5.2f marker %-5s %s' % (g, mk, cell(d3.run(disp='vrr', dxvk='fifo', vs=True, g=g, marker=mk), 'follow', 40, sd('bandv', g, mk))))

print('\n3. Sweep 2 of d3-sim.py: GPU = 1.25 T, vsync on, wake jitter x1 | x4 | x8 (20 windows), V1 and V2 only')
for hz in (60.0, 120.0, 165.0, 240.0, 360.0):
    T = 1000.0 / hz
    for disp, truth in (('vrr', 'follow'), ('fixed', 'grid')):
        cells = []
        for wsc in (1.0, 4.0, 8.0):
            pr = d3.run(hz=hz, disp=disp, dxvk='fifo', vs=True, g=1.25 * T, cr=0.5 * T, cw=0.4 * T, g_sd=0.06 * T, g_white=0.025 * T, cr_sd=0.05 * T, cw_sd=0.04 * T, wake_scale=wsc)
            cells.append(cell(pr, truth, 20, sd('sw', hz, disp, wsc), variants=(1, 2)))
        print('   %3.0f %-5s %s' % (hz, disp, '  ||  '.join(cells)))

print('\n4. d3-stress.py\'s cases (30 windows a cell), totals only')
t0 = {k: list(v) for k, v in TOT.items()}
for kind in ('dxvk', 'ext'):
    for cap in (40, 60, 70, 90, 100, 110):
        for gg0 in (0.6, 0.9, 0.97, 1.02, 1.1):
            gg = gg0 * 1000.0 / cap
            for dsp, tr in (('fixed', 'grid'), ('vrr', 'follow')):
                cell(d3.run(disp=dsp, dxvk='fifo', vs=True, g=gg, cr=min(5.0, 0.5 * gg), cw=min(4.0, 0.4 * gg), limiter=(kind, cap)), tr, 30, sd('A', kind, cap, gg0, dsp))
for pw, pj in ((0.05, 0), (2.0, 0), (4.0, 0), (8.3, 0), (10.0, 0), (0.05, 1.0), (0.05, 3.0), (0.05, 5.0)):
    for dsp, tr in (('fixed', 'grid'), ('vrr', 'follow')):
        cell(d3.run(disp=dsp, dxvk='fifo', vs=True, pw_off=pw, pw_jit=pj), tr, 30, sd('B', pw, pj, dsp))
for k in (2, 3):
    for s_ in (0.01, 0.03, 0.1, 0.3, 0.5):
        cell(d3.run(disp='vrr', dxvk='fifo', vs=True, g=k * 1000.0 / 120.0 - 0.12, g_sd=s_, g_white=s_ * 0.8), 'follow', 30, sd('C', k, s_))
for cr in (8.7, 9.5, 11.0, 14.0, 17.0, 20.0):
    for dsp, tr in (('fixed', 'grid'), ('vrr', 'follow')):
        cell(d3.run(disp=dsp, dxvk='fifo', vs=True, g=6.0, cr=cr, cw=4.0), tr, 30, sd('D1', cr, dsp))
        cell(d3.run(disp=dsp, dxvk='fifo', vs=True, g=6.0, cr=4.0, cw=cr), tr, 30, sd('D2', cr, dsp))
for vv in (0, 1, 2, 3):
    print('   V%d: %d windows, %d hard errors' % (vv, TOT[vv][0] - t0[vv][0], TOT[vv][1] - t0[vv][1]))

print('\n5. A session (J1 P5): 3 windows of G-Sync + driver vsync above the refresh, then 30 windows vsync off at 77 fps on VRR')
for vv in (0, 1):
    a = run2(d3.run(disp='vrr', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5), 3, 91, vv)
    b = run2(d3.run(disp='vrr', dxvk='imm', vs=False, g=13.0), 30, 92, vv)
    mem, held = memory(a + b)
    print('   V%d verdicts %s  memory at the end: %s' % (vv, ''.join(v[0] for v, _, _, _ in a + b), mem))

print('\n6. The half-period reason: a fixed 120 Hz panel read as 60 Hz and as 240 Hz (the refresh read half or double)')
for hz, rd, gg in ((120.0, 60.0, 10.0), (120.0, 60.0, 20.0), (120.0, 60.0, 25.0), (120.0, 240.0, 10.0), (60.0, 120.0, 20.0)):
    p = d3.run(hz=hz, disp='fixed', dxvk='fifo', vs=True, g=gg)
    res = run2(p, 20, sd('half', hz, rd), 1, T_read=1000.0 / rd)
    whys = {}
    for v, why, ft, _ in res: whys[why] = whys.get(why, 0) + 1
    print('   true %3.0f read %3.0f GPU %4.1f ms  %s' % (hz, rd, gg, '; '.join('%s %d' % kv for kv in whys.items())))

if DUMP:   # the V2 copies of sections 5 and 6 for the replay: the session continuous, the half-period cells as V2
    st = run2_dump(d3.run(disp='vrr', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5), 3, 91, truth='follow', tag='session-a')[1]
    run2_dump(d3.run(disp='vrr', dxvk='imm', vs=False, g=13.0), 30, 92, truth='follow', state=st, cont=True, tag='session-b')
    for hz, rd, gg in ((120.0, 60.0, 10.0), (120.0, 60.0, 20.0), (120.0, 60.0, 25.0), (120.0, 240.0, 10.0), (60.0, 120.0, 20.0)):
        run2_dump(d3.run(hz=hz, disp='fixed', dxvk='fifo', vs=True, g=gg), 20, sd('half', hz, rd), T_read=1000.0 / rd, truth='grid', tag='half')
    DF.close()
    print('\nDUMP %s: %d cells, %d V2 windows; on the rounded stamps %d hard errors, %d windows with a wrong memory; %d verdicts the rounding changed' % (
        DUMP, DSTAT['cells'], DSTAT['windows'], DSTAT['hard'], DSTAT['membad'], DSTAT['rounding_changed']))
print('\nTOTAL over everything above: ' + '; '.join('V%d %d windows, %d hard errors, %d cells whose memory held a wrong class' % (vv, TOT[vv][0], TOT[vv][1], MEMBAD[vv]) for vv in (0, 1, 2, 3)))
