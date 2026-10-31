# d3-sim.py: designer D3 (the sceptic), phase 1B of DLSS-IV's frame generation revival, 2026-09-24.
#
# Generates per-frame timing for the seven labelled F1B runs (and the sceptic's extra fooling cases) from the
# presentation physics in r3-dxvk.md and the mod's own stamps in r1-timing.md, then runs the proposed classifier on
# windows of 120 accepted frames and prints the verdicts and the margins. Then two sweeps: the frame rate across the
# frame generation band, and the refresh rate, each for a VRR panel and a fixed one, with the release jitter scaled.
#
# What is modelled, per frame f (all times in ms on one clock, the QPC):
#   render thread: b(f) = its begin stamp (latBeginQpc), the return of Present(f-1); call(f) = its Present call.
#     FusionFix's limiter (a game routine, before the call): rel = max(arrive, rel_prev + 1000/F) + overshoot
#     (framelimit.ixx:87-90, its deadline runs from its last release).
#     An unknown CPU limiter (RTSS-like, before our hook): DXVK's banked-deadline formula on the render thread.
#   worker (threaded=1): replays frame f, then DXVK's Present(f): the acquire (waits for disp(f-2): 3 Vulkan images),
#     then SyncFrameLatency with the queue of one (waits for sig(f-1)); the frame-end marker is submitted after it
#     (LatPresentObs runs before dtPresDone moves). The render thread returns from dtScPresent when the worker has
#     finished Present(f-1) (DT_R_PACE, dt_res.h:572). So b(x) waits for sig(x-3): the anchor lag is 3.
#   threaded=0: the render thread calls DXVK's Present itself: b(x) waits for sig(x-2), lag 2.
#   GPU: frame f starts when f-1 ends and half of f's replay has been submitted; runs g(f) (AR(1) scene drift plus
#     white noise).
#   display: VRR (shown when ready, no faster than the panel's maximum), fixed (next distinct vblank), composed
#     (next vblank, then one more for DWM), tear (shown at once), relaxed (FIFO_RELAXED), per r3 section 3.
#   DXVK's signal: FIFO waits for the present to be displayed (present wait), IMMEDIATE only for the GPU; DXVK's own
#     limiter (maxFrameRate, or the -refresh/2 limiter of a present interval of 2) delays the signal
#     (util_fps_limiter.cpp:74-76 banking).
#   the waiter: tEnd(f) = the marker's retire (max of the GPU end and the worker's Present return), tIn(f) = our
#     injection's retire (the GPU end minus a 1.2 ms post chain), each plus a wake.
# Thread wakes: 12 us plus an exponential of mean 25 us, and a 2 percent chance of a 0.2 to 1.5 ms scheduler delay;
# wake_scale multiplies both the mean and the chance.
#
# NOT modelled: the mod's pacer (it stands down under vsync and a limiter, and the design refuses windows where it
# slept); DXVK's CS-thread lag; the driver's own present queue beyond the 3 images; LFC below the VRR floor.
#
# Run with: python <this file>        (never python -)

import math
import numpy as np

LAT_WIN = 120

# ---------------------------------------------------------------- the classifier (mirrors the C design in d3-design.md)
C = dict(
    R_HI=0.80,        # phase lock at or above this: the presents sit on the refresh grid
    R_LO=0.40,        # phase lock at or below this: they do not (FREE also needs the anchor)
    G_HI=0.50,        # whole-refresh fraction QUANTISED needs
    EPS_G=0.10,       # an interval is a whole number of refreshes when within 0.10 T of one
    MIX=0.08,         # a second multiple holding 8 percent of intervals: a mix, which no limiter makes
    CEIL=1.03,        # mean interval at or under 1.03 T: at the refresh ceiling
    PERIOD_K=0.90,    # one multiple holds 90 percent: periodic (a cap at Hz/k looks the same) ...
    GRID_SPREAD=0.40, # an anchor spread over 0.40 T: released somewhere on a grid (a reason in the log only)
    PIN_IQR=0.03,     # interquartile range under 3 percent of the median: pinned by something
    MIN_N=96,         # consecutive intervals a window needs
    SLOW_FPS=32.0,    # below this the window is a load, a menu or a runtime cap (FusionFix's 30)
    HITCH_X=4.0, HITCH_FRAC=0.10,
    PACER_FRAC=0.10,  # the pacer slept on more than a tenth of presents: the begin stamps are its own
    LIM_TOL=0.03,     # a known cap binds when the median interval is within 3 percent of 1000/F
    BLOCK_PIN=0.10,   # vsync off, pinned at T: the wait must be inside DXVK's Present, at least 0.10 T a present
    BLOCK_FREE=0.25,  # the worker blocked at least a quarter of the period inside DXVK's Present
    A_LATE=1.25,      # the anchor bound for a blocked, unpinned window (a late but steady present wait)
)

def a_lim(T, marker):
    if marker:
        return min(max(0.30 * T, 1.5), 0.60 * T)
    return min(max(0.45 * T, 3.0), 0.70 * T)

def s_lim(T, marker):
    return (0.20 if marker else 0.25) * T

def features(w, T):
    iv = np.asarray(w['iv']); A = np.asarray(w['A']); b = np.asarray(w['b'])
    n = len(iv)
    mean_iv = float(np.mean(iv)); med = float(np.median(iv))
    q25, q75 = np.percentile(iv, [25, 75]); iqr = float(q75 - q25)
    # R from a 32-bin histogram of the phase, as the C code does it (no CRT trig is linked in the mod today): the bin
    # centres' cosines and sines come from a table; binning costs a factor sinc(pi/32) = 0.998
    bins = np.floor(np.mod((b - b[0]) / T, 1.0) * 32.0).astype(int) & 31
    cnt = np.bincount(bins, minlength=32)
    ang = 2 * math.pi * (np.arange(32) + 0.5) / 32.0
    R = float(abs(np.sum(cnt * np.exp(1j * ang)))) / len(b)
    k = np.rint(iv / T); r = iv / T - k
    on = (np.abs(r) <= C['EPS_G']) & (k >= 1)
    G = float(np.mean(on))
    if on.any():
        ks, cnt = np.unique(k[on], return_counts=True); order = np.argsort(-cnt)
        kmode = int(ks[order[0]]); kfrac = float(cnt[order[0]]) / n
        mix2 = float(cnt[order[1]]) / n if len(cnt) > 1 else 0.0
    else:
        kmode, kfrac, mix2 = 0, 0.0, 0.0
    Amed = float(np.median(A)); p10, p80 = np.percentile(A, [10, 80]); SA = float(p80 - p10)
    hitch = float(np.mean(iv > C['HITCH_X'] * med))
    return dict(n=n, mean=mean_iv, med=med, iqr=iqr, R=R, G=G, kmode=kmode, kfrac=kfrac, mix2=mix2, Amed=Amed, SA=SA,
                hitch=hitch, rate=1000.0 / mean_iv, AmedT=Amed / T, SAT=SA / T)

def classify(w, T, vs, F_cands, marker, block, pacer_frac):
    """Returns (verdict, why, features). T in ms (0: the refresh is unknown)."""
    if not T:
        return 'UNCERTAIN', 'the refresh is unknown', {}
    if len(w['iv']) < C['MIN_N']:
        return 'UNCERTAIN', 'too few consecutive intervals', {'n': len(w['iv'])}
    ft = features(w, T); ft['block'] = block; ft['blockP'] = block / ft['mean']
    mean_iv, med, iqr, R, G = ft['mean'], ft['med'], ft['iqr'], ft['R'], ft['G']
    if ft['rate'] < C['SLOW_FPS']:
        return 'UNCERTAIN', 'below 32 fps: a load, a menu or a runtime cap', ft
    if ft['hitch'] > C['HITCH_FRAC']:
        return 'UNCERTAIN', 'hitches', ft
    ceiling = mean_iv <= C['CEIL'] * T
    periodic = ft['kfrac'] >= C['PERIOD_K'] and ft['mix2'] < C['MIX']
    pinned = iqr <= C['PIN_IQR'] * med
    grid_anchor = ft['Amed'] <= T and ft['SA'] >= C['GRID_SPREAD'] * T
    fmatch = None
    for F in F_cands:
        P = 1000.0 / F
        if abs(med - P) <= max(C['LIM_TOL'] * P, 0.25) and iqr <= 0.10 * P:
            fmatch = F
    # 1. QUANTISED: phase-locked to the grid in whole refreshes, below the ceiling, whatever latVsync says, and a MIX of
    #    multiples, which no limiter makes. (A first cut also let a single multiple through when the anchor spread
    #    widely, as a display-bound fixed panel's does; d3-probe.py showed a banked limiter at Hz/2 that slips onto the
    #    GPU on a VRR panel spreads the anchor just as widely, 0.8 to 1.8 T, so that escape is gone: one multiple is
    #    UNCERTAIN, always.)
    if not ceiling and R >= C['R_HI'] and G >= C['G_HI'] and ft['mix2'] >= C['MIX']:
        return 'QUANTISED', 'a phase-locked mix of whole refreshes', ft
    # 2. vsync off at the game, yet pinned at one refresh, locked to its phase, with the wait inside DXVK's Present
    if (not vs) and abs(mean_iv - T) <= 0.015 * T and R >= C['R_HI'] and fmatch is None:
        if pacer_frac > C['PACER_FRAC']:
            return 'UNCERTAIN', 'the pacer slept: the begin stamps are its own clock', ft
        if block >= C['BLOCK_PIN'] * T:
            return 'QUANTISED', 'pinned at the refresh with vsync off at the game: the driver or the compositor syncs', ft
        return 'UNCERTAIN', 'pinned at the refresh with vsync off but no wait inside Present: a limiter outside DXVK?', ft
    # 3. LIMITED: a cap the mod knows binds
    if fmatch is not None:
        return 'LIMITED', 'the known cap %g fps binds' % fmatch, ft
    # 4. vsync off at the game: only a window above the refresh can show that nothing syncs
    if not vs:
        if mean_iv <= 0.97 * T and not pinned:
            return 'FREE', 'above the refresh and unsynced', ft
        return 'UNCERTAIN', 'vsync off at the game: below the refresh a synced display does not reach Present', ft
    if ceiling:
        return 'UNCERTAIN', 'at the refresh ceiling (VRR pins there too)', ft
    # 5. FREE: vsync on, each release a steady short time after the GPU end of the frame it waited for, off the grid
    #    The anchor is either short (the release came right after the GPU end), or, when the worker demonstrably waited
    #    inside DXVK's Present and the intervals follow the GPU (not pinned), up to 1.25 T (a present wait that
    #    completes late but steadily, which NVIDIA's driver may do: UNKNOWN)
    blocked = block >= C['BLOCK_FREE'] * mean_iv
    anchored = ft['Amed'] <= a_lim(T, marker) or (blocked and not pinned and ft['Amed'] <= C['A_LATE'] * T)
    if R <= C['R_LO'] and anchored and ft['SA'] <= s_lim(T, marker) and (marker or blocked):
        return 'FREE', 'released a steady time after the GPU end, off the grid' + ('' if ft['Amed'] <= a_lim(T, marker) else ' (a late present wait, the worker blocked)'), ft
    if periodic:
        return 'UNCERTAIN', 'periodic at %d refreshes (a cap at Hz/%d looks the same)' % (ft['kmode'], ft['kmode']), ft
    if pinned:
        return 'UNCERTAIN', 'pinned at %.1f fps by a limiter the mod does not know' % (1000.0 / med), ft
    if R >= 0.5 and ft['SA'] >= C['GRID_SPREAD'] * T:
        return 'UNCERTAIN', 'near the grid, too much release jitter to call', ft
    if ft['Amed'] > a_lim(T, marker):
        return 'UNCERTAIN', 'the presents did not wait for the display', ft
    return 'UNCERTAIN', 'between the two shapes', ft

# ---------------------------------------------------------------- the physics
def simulate(p, nframes, seed):
    rng = np.random.default_rng(seed)
    def wake(scale=1.0):
        w = 0.012 + rng.exponential(0.025 * scale)
        if rng.random() < min(0.5, 0.02 * scale):
            w += rng.uniform(0.2, 1.5)
        return w
    def ar(state, mean, sd, rho=0.95, white=0.0):
        state = rho * state + rng.normal(0.0, sd * math.sqrt(1 - rho * rho))
        return state, max(0.5, mean + state + (rng.normal(0.0, white) if white else 0.0))
    Hz = p['hz']; T = 1000.0 / Hz; Tt = p.get('T_true', T); Tmin = 1000.0 / p.get('vrr_max', Hz)
    ph0 = rng.uniform(0, Tt)
    disp_mode = p['disp']; fifo = p['dxvk'] == 'fifo'
    thr = p.get('threaded', 1); ws_ = p.get('wake_scale', 1.0)
    N = nframes
    b = np.zeros(N + 1); call = np.zeros(N); wdone = np.zeros(N); relW = np.zeros(N)
    g_end = np.zeros(N); disp = np.zeros(N); sig = np.zeros(N); tEnd = np.zeros(N); tIn = np.zeros(N); blk = np.zeros(N)
    gs = cs = wsd = 0.0
    ff_rel = -1e9; ext_next = 0.0; dx_next = 0.0
    lim = p.get('limiter')
    def vb(t):
        return ph0 + math.ceil((t - ph0) / Tt) * Tt
    for f in range(N):
        gs, g = ar(gs, p['g'], p.get('g_sd', 0.6), white=p.get('g_white', 0.25))
        cs, cr = ar(cs, p['cr'], p.get('cr_sd', 0.5), white=0.2)
        wsd, cw = ar(wsd, p['cw'], p.get('cw_sd', 0.4), white=0.15)
        if lim and lim[0] == 'ff':
            P = 1000.0 / lim[1]
            rel = max(b[f] + 0.5 * cr, ff_rel + P) + 0.005 + rng.exponential(0.02)
            ff_rel = rel; call[f] = rel + 0.5 * cr
        elif lim and lim[0] == 'ext':
            P = 1000.0 / lim[1]
            t1 = b[f] + cr; rel = max(t1, ext_next)
            ext_next = ext_next + P if rel < ext_next + P else rel + P
            call[f] = rel + 0.003 + rng.exponential(0.005)
        else:
            call[f] = b[f] + cr
        if thr:
            wc = max(call[f] + 0.05, (wdone[f - 1] if f else 0.0) + cw); first = wc - 0.5 * cw
        else:
            wc = call[f]; first = call[f] - 0.5 * cr
        g_end[f] = max(g_end[f - 1] if f else 0.0, first) + g
        ready = max(g_end[f] + 0.1, wc + 0.15) + abs(rng.normal(0, 0.02))
        prev = disp[f - 1] if f else -1e9
        if disp_mode == 'vrr':
            d = max(ready + 0.08, prev + Tmin)
        elif disp_mode == 'fixed':
            d = vb(max(ready, prev + 0.5 * Tt))
        elif disp_mode == 'composed':
            d = vb(ready) + Tt
            if d <= prev + 0.5 * Tt: d = prev + Tt
        elif disp_mode == 'tear':
            d = ready
        elif disp_mode == 'relaxed':
            d = ready if ready > prev + Tt else vb(max(ready, prev + 0.5 * Tt))
        else:
            raise ValueError(disp_mode)
        disp[f] = d
        # present wait completes pw_off after the flip (0.05 by default), plus pw_jit of uniform jitter: both UNKNOWN on
        # NVIDIA's driver, so the sceptic's cases move them
        pw = p.get('pw_off', 0.05) + (rng.uniform(0, p['pw_jit']) if p.get('pw_jit') else 0.0)
        s_raw = max(g_end[f] + 0.02, disp[f] + pw + wake(ws_)) if fifo else g_end[f] + 0.02 + 0.5 * wake(ws_)
        if lim and lim[0] == 'dxvk':
            P = 1000.0 / lim[1]
            s_rel = max(s_raw, dx_next)
            dx_next = dx_next + P if s_rel < dx_next + P else s_rel + P
            s_raw = s_rel + 0.01
        sig[f] = s_raw
        acq = wc + 0.05
        if f >= 2: acq = max(acq, disp[f - 2] + 0.05 + wake(ws_))
        done = max(acq + 0.08, (sig[f - 1] + wake(ws_)) if f else 0.0)
        blk[f] = done - wc
        wdone[f] = done
        if thr:
            relW[f] = done + 0.012
            tEnd[f] = max(g_end[f] + 0.02, done + 0.015) + 0.8 * wake(ws_)
            b[f + 1] = max(call[f] + 0.02, (relW[f - 1] if f else 0.0) + wake(ws_)) + 0.01
        else:
            tEnd[f] = max(g_end[f] + 0.02, done + 0.03) + 0.8 * wake(ws_)
            b[f + 1] = done + 0.03
        tIn[f] = g_end[f] - max(0.3, rng.normal(1.2, 0.15)) + 0.8 * wake(ws_)
    return dict(b=b, tEnd=tEnd, tIn=tIn, blk=blk)

def windows(sim, p, drop=0.02, seed=0):
    rng = np.random.default_rng(seed + 7)
    b = sim['b']; blk = sim['blk']
    marker = p.get('marker', True)
    E = sim['tEnd'] if marker else sim['tIn']
    lag = 3 if p.get('threaded', 1) else 2
    out = []; cur = dict(b=[], iv=[], A=[], blk=[]); prev_ok = None
    for x in range(200, len(b) - 1):
        if rng.random() < drop:                  # a frame that fails the begin-stamp pairing: dropped, uncounted
            prev_ok = None; continue
        cur['b'].append(b[x])
        if prev_ok == x - 1: cur['iv'].append(b[x] - b[x - 1])
        cur['A'].append(b[x] - E[x - lag]); cur['blk'].append(blk[x - 1]); prev_ok = x
        if len(cur['b']) == LAT_WIN:
            out.append(cur); cur = dict(b=[], iv=[], A=[], blk=[])
    return out

def run_windows(p, nw, seed, T_read=None):
    sim = simulate(p, nw * LAT_WIN + 400, seed)
    T = T_read if T_read is not None else 1000.0 / p['hz']
    res = []
    for w in windows(sim, p, seed=seed)[:nw]:
        block = float(np.mean(w['blk']))
        F = [p['limiter_known']] if p.get('limiter_known') else []
        res.append(classify(w, T, p['vs'], F, p.get('marker', True), block, 0.0))
    return res

# ---------------------------------------------------------------- the runs
BASE = dict(hz=120.0, g=10.0, cr=5.0, cw=4.0, threaded=1, marker=True)
def run(**kw):
    d = dict(BASE); d.update(kw); return d

# truth: 'follow' = the screen shows each present when it is made (VRR, or tearing); 'grid' = the screen quantises.
# A hard error is QUANTISED on 'follow' or FREE on 'grid'. LIMITED and UNCERTAIN are never hard errors.
RUNS = [
    ('F1B-1 G-Sync on, vsync on',                    'FREE',      'follow', run(disp='vrr', dxvk='fifo', vs=True)),
    ('F1B-2 vsync only, fixed refresh',              'QUANTISED', 'grid',   run(disp='fixed', dxvk='fifo', vs=True)),
    ('F1B-3 neither, below the refresh',             'UNCERTAIN', 'follow', run(disp='tear', dxvk='imm', vs=False)),
    ('F1B-3+ neither, a light scene above refresh',  'FREE',      'follow', run(disp='tear', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5)),
    ('F1B-4 FS-only G-Sync, borderless (fixed)',     'QUANTISED', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=11.0)),
    ('F1B-5 driver vsync, game off, below refresh',  'UNCERTAIN', 'grid',   run(disp='fixed', dxvk='imm', vs=False)),
    ('F1B-5+ driver vsync, above refresh',           'QUANTISED', 'grid',   run(disp='fixed', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5)),
    ('F1B-6 composed at a fixed rate, vsync on',     'QUANTISED', 'grid',   run(disp='composed', dxvk='fifo', vs=True)),
    ('F1B-7 FusionFix 60 on VRR, vsync on',          'LIMITED',   'follow', run(disp='vrr', dxvk='fifo', vs=True, limiter=('ff', 60), limiter_known=60, marker=False)),
    ('S1 VRR, GPU at 2T by chance (sd 0.10)',        'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=16.667 - 0.12, g_sd=0.10, g_white=0.08)),
    ('S1b VRR, GPU at 2T, very steady (sd 0.02)',    'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=16.667 - 0.12, g_sd=0.02, g_white=0.02)),
    ('S2 VRR + unknown CPU cap 60 (RTSS-like)',      'UNCERTAIN', 'follow', run(disp='vrr', dxvk='fifo', vs=True, limiter=('ext', 60))),
    ('S3 fixed + FusionFix 60, vsync on',            'LIMITED',   'grid',   run(disp='fixed', dxvk='fifo', vs=True, limiter=('ff', 60), limiter_known=60, marker=False)),
    ('S4 fixed FIFO, render-bound (14 ms)',          'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=6.0, cr=14.0, cw=4.0)),
    ('S5 VRR FIFO, render-bound (14 ms)',            'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=6.0, cr=14.0, cw=4.0)),
    ('S6 fixed FIFO, wake jitter x8',                'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, wake_scale=8.0)),
    ('S6b VRR FIFO, wake jitter x8',                 'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, wake_scale=8.0)),
    ('S7 VRR FIFO at the ceiling (GPU 7 ms)',        'UNCERTAIN', 'follow', run(disp='vrr', dxvk='fifo', vs=True, g=7.0, cr=4.0, cw=3.0)),
    ('S8 fixed at 119.88, refresh read as 120',      'QUANTISED', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, T_true=1000.0 / 119.88)),
    ('S9 VRR + FusionFix 100, vsync on',             'LIMITED',   'follow', run(disp='vrr', dxvk='fifo', vs=True, g=8.5, cr=4.0, cw=3.0, limiter=('ff', 100), limiter_known=100, marker=False)),
    ('S10 fixed FIFO, GPU 13 ms (mostly 2T)',        'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=13.0, g_sd=0.3)),
    ('S10b fixed FIFO, GPU 14 ms steady (all 2T)',   'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=14.0, g_sd=0.1, g_white=0.1)),
    ('S11 VRR + DXVK half-rate limiter (60)',        'UNCERTAIN', 'follow', run(disp='vrr', dxvk='fifo', vs=True, limiter=('dxvk', 60))),
    ('S11b fixed + DXVK half-rate limiter (60)',     'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, limiter=('dxvk', 60))),
    ('S12 fixed + FusionFix 100',                    'LIMITED',   'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=8.0, cr=4.0, cw=3.0, limiter=('ff', 100), limiter_known=100, marker=False)),
    ('S13 VRR vsync on, threaded=0',                 'FREE',      'follow', run(disp='vrr', dxvk='fifo', vs=True, threaded=0)),
    ('S14 fixed vsync on, threaded=0',               'QUANTISED', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, threaded=0)),
    ('S15 240 Hz VRR, vsync on, GPU 6 ms',           'FREE',      'follow', run(hz=240.0, disp='vrr', dxvk='fifo', vs=True, g=6.0, cr=3.0, cw=2.5, g_sd=0.35, g_white=0.15)),
    ('S16 240 Hz fixed, vsync on, GPU 6 ms',         'QUANT|UNC', 'grid',   run(hz=240.0, disp='fixed', dxvk='fifo', vs=True, g=6.0, cr=3.0, cw=2.5, g_sd=0.35, g_white=0.15)),
    ('S17 VRR, vsync off (the owner), below',        'UNCERTAIN', 'follow', run(disp='tear', dxvk='imm', vs=False)),
    ('S17+ VRR, vsync off, above the refresh',       'FREE',      'follow', run(disp='tear', dxvk='imm', vs=False, g=5.0, cr=3.0, cw=2.5)),
    ('S18 FIFO_RELAXED fixed (tearFree=False)',      'FREE|UNC',  'follow', run(disp='relaxed', dxvk='fifo', vs=True)),
    ('S19 VRR FIFO, no marker (pacer=0)',            'FREE',      'follow', run(disp='vrr', dxvk='fifo', vs=True, marker=False)),
    ('S20 fixed FIFO, no marker (pacer=0)',          'QUANTISED', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, marker=False)),
    ('S21 VRR FIFO, worker-bound 8.9 ms',            'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=6.0, cr=4.0, cw=8.9)),
    ('S22 fixed FIFO, worker-bound 8.9 ms',          'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=6.0, cr=4.0, cw=8.9)),
    ('S23 VRR FIFO, GPU 16.2 ms (near 2T)',          'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=16.2)),
    ('S24 fixed FIFO, GPU 15 ms',                    'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=15.0)),
    ('S25 VRR, menu cap 50 but the file says 60',    'UNCERTAIN', 'follow', run(disp='vrr', dxvk='fifo', vs=True, limiter=('ff', 50), limiter_known=60, marker=False)),
    ('S26 VRR, FusionFix loading cap 30',            'UNCERTAIN', 'follow', run(disp='vrr', dxvk='fifo', vs=True, limiter=('ff', 30), limiter_known=60, marker=False)),
    ('S27 fixed FIFO, refresh unknown',              'UNCERTAIN', 'grid',   dict(run(disp='fixed', dxvk='fifo', vs=True), T_read=0)),
    ('S28 VRR, present wait ends a scan later',      'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, pw_off=8.3)),
    ('S29 VRR, present wait jitter 0-2 ms',          'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, pw_jit=2.0)),
    ('S30 fixed, present wait jitter 0-2 ms',        'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, pw_jit=2.0)),
    ('S31 fixed, present wait ends a scan later',    'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, pw_off=8.3)),
    ('S32 VRR + DXVK cap 100 the mod missed',        'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=8.5, cr=4.0, cw=3.0, limiter=('dxvk', 100))),
    ('S33 VRR + DXVK cap 100, known',                'LIMITED',   'follow', run(disp='vrr', dxvk='fifo', vs=True, g=8.5, cr=4.0, cw=3.0, limiter=('dxvk', 100), limiter_known=100)),
    ('S34 fixed + DXVK cap 100 the mod missed',      'QUANT|UNC', 'grid',   run(disp='fixed', dxvk='fifo', vs=True, g=8.5, cr=4.0, cw=3.0, limiter=('dxvk', 100))),
    ('S35 VRR + DXVK cap 70 the mod missed, GPU 12', 'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, g=12.0, limiter=('dxvk', 70))),
    ('S36 fixed + DXVK cap 70 the mod missed, GPU 12', 'QUANT|UNC', 'grid', run(disp='fixed', dxvk='fifo', vs=True, g=12.0, limiter=('dxvk', 70))),
    ('S37 VRR, late present wait, steady GPU 2T',    'FREE|UNC',  'follow', run(disp='vrr', dxvk='fifo', vs=True, pw_off=8.3, g=16.667 - 0.12, g_sd=0.02, g_white=0.02)),
]

def allowed(expected, v):
    alts = {'UNC': 'UNCERTAIN', 'QUANT': 'QUANTISED'}
    return v in [alts.get(e, e) for e in expected.split('|')]

def hard_error(truth, v):
    return (truth == 'follow' and v == 'QUANTISED') or (truth == 'grid' and v == 'FREE')

def fmt_range(a):
    return ('%.2f-%.2f' % (min(a), max(a))) if a else '-'

def main():
    NW = 40
    print('D3 sceptic classifier, synthetic runs: %d windows of %d accepted frames each (2 percent of frames dropped unpaired)\n' % (NW, LAT_WIN))
    hdr = '%-46s %-10s | %-36s | %-10s %-10s %-10s %-10s %-10s %-11s' % ('run', 'expected', 'verdicts', 'phase R', 'whole G', 'anchor/T', 'spread/T', 'block/P', 'fps')
    print(hdr); print('-' * len(hdr))
    bad = hard = 0
    for i, (name, exp, truth, p) in enumerate(RUNS):
        res = run_windows(p, NW, 1000 + i, T_read=p.get('T_read'))
        counts = {}; reasons = {}; Rs = []; Gs = []; As = []; Ss = []; fps = []; Bs = []
        nb = nh = 0
        for v, why, ft in res:
            counts[v] = counts.get(v, 0) + 1; reasons[why] = reasons.get(why, 0) + 1
            if 'R' in ft:
                Rs.append(ft['R']); Gs.append(ft['G']); As.append(ft['AmedT']); Ss.append(ft['SAT']); fps.append(ft['rate']); Bs.append(ft['blockP'])
            nb += 0 if allowed(exp, v) else 1; nh += 1 if hard_error(truth, v) else 0
        bad += nb; hard += nh
        cs = ', '.join('%s %d' % (k, counts[k]) for k in ('FREE', 'QUANTISED', 'LIMITED', 'UNCERTAIN') if k in counts)
        flag = ('  <-- %d unexpected' % nb if nb else '') + (' (%d HARD ERRORS)' % nh if nh else '')
        print('%-46s %-10s | %-36s | %-10s %-10s %-10s %-10s %-10s %-11s%s' % (name, exp, cs, fmt_range(Rs), fmt_range(Gs), fmt_range(As), fmt_range(Ss), fmt_range(Bs), fmt_range(fps), flag))
        top = sorted(reasons.items(), key=lambda kv: -kv[1])[:2]
        print('%-46s   why: %s' % ('', '; '.join('%s (%d)' % kv for kv in top)))
    print('\nwindows not as expected: %d; HARD ERRORS (QUANTISED where the screen follows presents, FREE where it quantises): %d' % (bad, hard))

    # sweep 1: the frame rate across the FG band and beyond, 120 Hz, VRR and fixed, vsync on
    print('\nSweep 1, 120 Hz, vsync on, GPU-bound: verdicts over 20 windows per point (F=FREE Q=QUANTISED L=LIMITED U=UNCERTAIN)')
    print('%-8s %-24s %-24s' % ('GPU ms', 'VRR', 'fixed'))
    for g in (22.0, 20.0, 18.0, 16.6, 16.0, 15.0, 14.0, 13.0, 12.0, 11.0, 10.0, 9.0, 8.4, 7.5):
        row = []
        for disp in ('vrr', 'fixed'):
            res = run_windows(run(disp=disp, dxvk='fifo', vs=True, g=g), 20, int(g * 100) + (1 if disp == 'fixed' else 0))
            c = {}
            for v, _, _ in res: c[v[0]] = c.get(v[0], 0) + 1
            row.append(' '.join('%s%d' % (k, c[k]) for k in 'FQLU' if k in c))
        print('%-8.1f %-24s %-24s' % (g, row[0], row[1]))

    # sweep 2: the refresh rate, GPU at 0.8 of the refresh rate, and the wake jitter scaled
    print('\nSweep 2, GPU frame = 1.25 T, vsync on: verdicts over 20 windows per point, at wake jitter x1, x4 and x8')
    print('%-6s %-28s %-28s' % ('Hz', 'VRR (x1 | x4 | x8)', 'fixed (x1 | x4 | x8)'))
    for hz in (60.0, 100.0, 120.0, 144.0, 165.0, 240.0, 360.0):
        T = 1000.0 / hz; row = []
        for disp in ('vrr', 'fixed'):
            cells = []
            for wsc in (1.0, 4.0, 8.0):
                pr = run(hz=hz, disp=disp, dxvk='fifo', vs=True, g=1.25 * T, cr=0.5 * T, cw=0.4 * T, g_sd=0.06 * T, g_white=0.025 * T, cr_sd=0.05 * T, cw_sd=0.04 * T, wake_scale=wsc)
                res = run_windows(pr, 20, int(hz) * 10 + int(wsc) + (5 if disp == 'fixed' else 0))
                c = {}
                for v, _, _ in res: c[v[0]] = c.get(v[0], 0) + 1
                cells.append(''.join('%s%d' % (k, c[k]) for k in 'FQLU' if k in c))
            row.append(' | '.join(cells))
        print('%-6.0f %-28s %-28s' % (hz, row[0], row[1]))
    print('\nthresholds: ' + ', '.join('%s=%s' % kv for kv in C.items()))

if __name__ == '__main__':
    main()
