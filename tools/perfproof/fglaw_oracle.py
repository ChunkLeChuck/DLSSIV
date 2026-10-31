r"""THE PACING LAW'S ORACLE (2026-10-05): the law of source/fg/fg_law.h as the research round's critic wrote it, in Python,
before the C++ existed (docs/reports/fg-1c/law-2026-10-05/critic.txt, R1; its script k8_variant.py, class LC). Kept as it
was written but for: the branch counts; the constants, in a table a control may change; and four amendments, each marked
AMENDED below, that writing the C++ and its streams found:
  1. free_min is never under hold_max, so never under the floor. With a floor above it (LOCK 1.2 at 120 Hz is 20.3 ms, and
     free_min was 20.0) the way down to "the regime's edge" walked under the floor.
  2. the top is never under the floor, and a rise the top holds at the floor is no rise. A light game on a slow panel (a
     60 Hz floor is 33.8 ms; the median plus 18 can be 26) had a top under its floor: the way down took it as a period,
     and the rise, which the critic's lines then moved off the floor to the first place over it whatever the top said,
     came back down twelve frames later and rose again after six, for as long as the game stayed so.
  3. the 12 s memory holds 4096 samples, not 1024: at 1024 it is 12 s long only under 85 samples a second, and a 240 Hz
     panel's floor is 118.
  4. a way down always exists: a place in a LOWER regime is a reason to move down whatever the tolerance. The way down
     asked for a target under the period by more than the tolerance (3 percent, 0.35 ms at least); where the free range
     starts nearer the floor than that (LOCK 1.2: both at 20.0 ms; any panel whose free range starts within 0.6 ms of
     its floor) a period just above the floor could never come down, which is the old law's own defect over again.
AND ONE FORM ADDED (2026-10-17, phase 5, the pacer's package; fg_law.h's header, "THE FREE BAND'S FORM", has the why and the
numbers), `form=1`, which the merged path runs at a head of 1.0 (FgLawStepForm): the way up has no tolerance under the place
(up_tol_pm thousandths of the period, 0), and the way back to the floor asks back_pm thousandths (1060) of the half-rank level
to fit the floor, where form 0 asks the head; while it does not, the way down stops at the first place over the floor. Form 0
is the law above, line for line. SINCE THE OWNER'S EYE ON LEG 7 (2026-10-17, evening) the armed build's free range starts AT
THE FLOOR (free_min 0: AMENDED 1 puts it at the hold band's end, which is the floor with no band); nothing here changed for it,
and equiv_fglaw runs every stream in form 1 that way too.
tools/perfproof/equiv_fglaw.py runs the C++ against this, step by step, in both forms. It imports nothing of the mod's.

The law's own words are in fg_law.h's header. The names here are the critic's.
"""
import bisect, math
from collections import deque


class Ring:
    def __init__(s, win, cap): s.win = win; s.cap = cap; s.q = deque(); s.s = []
    def push(s, t, c):
        s.q.append((t, c)); bisect.insort(s.s, c)
        while s.q and (t - s.q[0][0] > s.win or len(s.q) > s.cap):
            _, old = s.q.popleft(); del s.s[bisect.bisect_left(s.s, old)]
    def trim(s, t):
        while s.q and t - s.q[0][0] > s.win:
            _, old = s.q.popleft(); del s.s[bisect.bisect_left(s.s, old)]
    def kth(s, k): return s.s[-k] if len(s.s) >= k else 0.0
    def n(s): return len(s.s)


# THE CONSTANTS A CONTROL MAY CHANGE, one at a time (equiv_fglaw.py's controls: the C++ must then DIFFER)
CONST = dict(hitch=1.5, door=8, open=240, fe=0.05, fl=0.02, ke_min=6, kl_min=5, up=6, down=12, tol_hold=0.15, step_hold=0.2,
             least_free=0.4, least_hold=0.2, band=0.03, band_min=0.35, top=18.0, top_max=40.0, fast=4000.0, slow=12000.0,
             fast_cap=1024, slow_cap=4096, warm=150, lo=0.2, hi=100.0, half=0.5,
             up_tol_pm=0, back_pm=1060)                      # (the two of form 1: fg_law.h's FG_LAW_UP_TOL_PM and FG_LAW_BACK_PM)


class LC:
    def __init__(s, floor, hold_max=None, free_min=20.0, head=1.06, k=None, form=0):
        s.k = dict(CONST); s.k.update(k or {}); s.form = form
        s.floor = floor; s.hold_max = hold_max if hold_max and hold_max > floor else floor; s.head = head
        s.free_min = free_min if free_min > s.hold_max else s.hold_max                           # AMENDED 1
        s.per = floor; s.warm = s.k['warm']; s.dwell = 0; s.moves = 0
        s.fast = Ring(s.k['fast'], s.k['fast_cap']); s.slowr = Ring(s.k['slow'], s.k['slow_cap'])   # AMENDED 3 (the slow cap)
        s.run = 0; s.shift_left = 0
        s.E = 0.0; s.L = 0.0; s.hitch = 0; s.top = 0.0
        s.hits = {}                                         # which branches a run took (a branch never taken proves nothing)
    def hit(s, name): s.hits[name] = s.hits.get(name, 0) + 1
    def snap(s, want):
        """The resting period for a wanted one: the floor, a step of the hold band, or the free range."""
        if want <= s.floor + 1e-9: return s.floor
        if want <= s.hold_max + 1e-9: return min(s.hold_max, s.floor + math.ceil((want - s.floor) / s.k['step_hold'] - 1e-9) * s.k['step_hold'])
        return max(s.free_min, want)
    def reg(s, p): return 0 if p <= s.floor + 1e-9 else (1 if p <= s.hold_max + 1e-9 else 2)
    def step(s, c, t):
        k = s.k
        s.hitch = 0
        if k['lo'] < c < k['hi']:
            hitch = c > k['hitch'] * s.per
            s.hitch = 1 if hitch else 0
            s.run = s.run + 1 if hitch else 0
            if s.run >= k['door']: s.shift_left = k['open']; s.hit('the door opened')
            if not hitch and s.shift_left > 0: s.shift_left -= 1
            if not hitch or s.shift_left > 0:
                if len(s.fast.q) >= s.fast.cap: s.hit('the fast count capped')
                if len(s.slowr.q) >= s.slowr.cap: s.hit('the slow count capped')
                s.fast.push(t, c); s.slowr.push(t, c)
                if hitch: s.hit('a hitch taken through the door')
            else: s.hit('a hitch left out')
        else: s.hit('a sample not taken')
        s.fast.trim(t); s.slowr.trim(t)
        n = s.fast.n(); m = s.slowr.n()
        Ke = max(k['ke_min'], int(math.ceil(k['fe'] * n))); Kl = max(k['kl_min'], int(math.ceil(k['fl'] * n)))
        E = s.fast.kth(Ke); Lv = max(s.fast.kth(Kl), E)
        Ed = s.fast.kth(max(3, Ke // 2)); Ld = max(s.fast.kth(max(2, Kl // 2)), Ed)
        Ks = max(k['ke_min'], int(math.ceil(k['fe'] * m))); Kls = max(k['kl_min'], int(math.ceil(k['fl'] * m)))
        Es = s.slowr.kth(max(3, Ks // 2)); Ls = max(s.slowr.kth(max(2, Kls // 2)), Es)
        med = s.fast.s[len(s.fast.s) // 2] if s.fast.s else 0.0
        s.E = E; s.L = Lv
        if s.warm > 0: s.warm -= 1; s.hit('a warm-up step'); return s.per
        s.dwell += 1
        P = s.per; F = s.floor; band = max(P * k['band'], k['band_min'])
        top = max(F, min(k['top_max'], med + k['top']) if med > 0 else k['top_max'])             # AMENDED 2
        if top <= F: s.hit('the top at the floor')
        s.top = top
        if P <= F + 1e-9:
            if E > F and s.dwell >= k['up']:
                want = s.snap(s.head * Lv)
                if want > top: s.hit('the top held a rise')
                to = min(top, want)
                if to > F + 1e-9:                                                                # AMENDED 2: else no place over the floor
                    s.per = to; s.dwell = 0; s.moves += 1; s.hit('a rise from the floor')
                    s.hit('a rise into the hold band' if s.reg(s.per) == 1 else 'a rise into the free range')
                else: s.hit('no place over the floor: it stays')
        else:
            up = min(top, s.snap(s.head * Lv))
            back = k['back_pm'] * 0.001
            if s.form:                                                                           # FORM 1: the back level, and never under the first place over the floor
                dn = min(top, s.snap(max(s.head * Ld, F + 1e-6))) if back * Ed > F else F
                if back * Ed > F and s.head * Ld <= F + 1e-9: s.hit('form 1: the way down held over the floor')
            else: dn = min(top, s.snap(s.head * Ld)) if s.head * Ed > F else F
            rP = s.reg(P)
            tol = k['tol_hold'] if rP == 1 else band
            utol = P * (k['up_tol_pm'] * 0.001) if s.form else (k['tol_hold'] if rP == 1 and s.reg(up) == 1 else band)   # FORM 1: no tolerance under the place
            if s.form and P < up <= P + (k['tol_hold'] if rP == 1 and s.reg(up) == 1 else band) and s.dwell >= k['up']: s.hit('form 1: up inside the old tolerance')
            if up > P + utol:
                if s.dwell >= k['up']:
                    s.per = up; s.dwell = 0; s.moves += 1
                    s.hit('up, inside the hold band' if rP == 1 and s.reg(up) == 1 else 'up, out of the hold band' if rP == 1 else 'up, in the free range')
            elif s.dwell >= k['down'] and (dn < P - tol or s.reg(dn) < rP):                        # AMENDED 4 (the second half)
                edge = s.free_min if rP == 2 else F + k['step_hold']
                least = k['least_free'] if rP == 2 else k['least_hold']
                if s.reg(dn) == rP:
                    s.per = max(dn, P - max(least, (P - dn) * k['half'])); s.dwell = 0; s.moves += 1
                    s.hit('down inside the hold band' if rP == 1 else 'down inside the free range')
                elif P > edge + tol:
                    s.per = max(edge, P - max(least, (P - edge) * k['half'])); s.dwell = 0; s.moves += 1
                    s.hit('down to the hold band\'s edge' if rP == 1 else 'down to the free range\'s edge')
                else:
                    if s.form: dns = min(top, s.snap(max(s.head * max(Ls, Ld), F + 1e-6))) if back * max(Es, Ed) > F else F
                    else: dns = min(top, s.snap(s.head * max(Ls, Ld))) if s.head * max(Es, Ed) > F else F
                    if s.reg(dns) < rP:
                        s.per = dns; s.dwell = 0; s.moves += 1
                        s.hit('across a band, to the floor' if s.reg(dns) == 0 else 'across a band, into the hold')
                    else: s.hit('at an edge, the slow memory said no')
        return s.per


# EVERY BRANCH THE LAW HAS, by the names above: a proof whose streams leave one untaken says so and fails
BRANCHES = ('the door opened', 'the fast count capped', 'the slow count capped', 'a hitch taken through the door', 'a hitch left out',
            'a sample not taken', 'a warm-up step', 'the top at the floor', 'the top held a rise', 'a rise from the floor',
            'no place over the floor: it stays', 'a rise into the hold band', 'a rise into the free range', 'up, inside the hold band', 'up, out of the hold band', 'up, in the free range',
            'down inside the hold band', 'down inside the free range', 'down to the hold band\'s edge', 'down to the free range\'s edge',
            'across a band, to the floor', 'across a band, into the hold', 'at an edge, the slow memory said no',
            'form 1: the way down held over the floor', 'form 1: up inside the old tolerance')


def loop(law, cost, cap=20.0):
    """The pacer's own step 4 around the law (fg_machine.h, Pace): arrival = release + cost; a resync, a clamp, or a sleep to
    the due (one sleep 20 ms at most, the due restarting where the cap ended); the due advances by the period. Returns, a
    frame: the sample, the time the law was given, the period it answered, its E and L, its regime, whether the sample was a
    hitch, and the top it used (0 in the warm-up)."""
    t = 0.0; due = None; out = []
    for c in cost:
        now = t + c
        law.top = 0.0
        P = law.step(c, now)
        out.append((c, now, P, law.E, law.L, law.reg(P), law.hitch, law.top))
        if due is None or now > due + min(4.0 * P, 120.0): due = now
        elif now > due: due = now
        elif due - now > 0.05:
            if due - now > cap: due = now + cap
            now = due
        due += P; t = now
    return out
