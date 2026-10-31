# d3-stress.py (D3, phase 1B, 2026-09-24): hunts hard errors in the thin places d3-sim.py's cases found. It imports
# d3-sim.py's model and classifier unchanged. Hard error = QUANTISED where the screen follows presents (VRR, or
# tearing), or FREE where the screen quantises. Run with: python d3-stress.py
import importlib.util
spec = importlib.util.spec_from_file_location('d3sim', 'd3-sim.py'); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
NW = 30
hard = 0; tot = 0; seed = 1
def cell(pr, truth):
    global hard, tot, seed
    seed += 1
    c = {}
    for v, _, _ in m.run_windows(pr, NW, 50000 + seed):
        c[v[0]] = c.get(v[0], 0) + 1; tot += 1
        if m.hard_error(truth, v): hard += 1
    return ' '.join('%s%d' % (k, c[k]) for k in 'FQLU' if k in c)

print('A. a limiter the mod does not know, GPU from 0.6 to 1.1 of its period, 120 Hz, vsync on (%d windows a cell)' % NW)
print('%-6s %-5s %-6s %-16s %-16s' % ('kind', 'cap', 'GPU', 'fixed', 'VRR'))
for kind in ('dxvk', 'ext'):
    for cap in (40, 60, 70, 90, 100, 110):
        for g in (0.6, 0.9, 0.97, 1.02, 1.1):
            gg = g * 1000.0 / cap
            row = [cell(m.run(disp=d, dxvk='fifo', vs=True, g=gg, cr=min(5.0, 0.5 * gg), cw=min(4.0, 0.4 * gg), limiter=(kind, cap)), t)
                   for d, t in (('fixed', 'grid'), ('vrr', 'follow'))]
            print('%-6s %-5d %-6.2f %-16s %-16s' % (kind, cap, gg, row[0], row[1]))
print('\nB. the present wait completes late (pw_off ms) or with jitter (pw_jit ms), 120 Hz, GPU 10 ms')
for pw, pj in ((0.05, 0), (2.0, 0), (4.0, 0), (8.3, 0), (10.0, 0), (0.05, 1.0), (0.05, 3.0), (0.05, 5.0)):
    row = [cell(m.run(disp=d, dxvk='fifo', vs=True, pw_off=pw, pw_jit=pj), t) for d, t in (('fixed', 'grid'), ('vrr', 'follow'))]
    print('pw_off %-5.2f pw_jit %-4.1f %-16s %-16s' % (pw, pj, row[0], row[1]))
print('\nC. VRR GPU-bound, the frame time a whole number of refreshes by chance, GPU noise sd from 0.01 to 0.5 ms')
for k in (2, 3):
    for sd in (0.01, 0.03, 0.1, 0.3, 0.5):
        g = k * 1000.0 / 120.0 - 0.12
        print('k=%d sd %-5.2f VRR %s' % (k, sd, cell(m.run(disp='vrr', dxvk='fifo', vs=True, g=g, g_sd=sd, g_white=sd * 0.8), 'follow')))
print('\nD. fixed, render-bound or worker-bound at rates from 50 to 115 fps')
for cr in (8.7, 9.5, 11.0, 14.0, 17.0, 20.0):
    row = [cell(m.run(disp=d, dxvk='fifo', vs=True, g=6.0, cr=cr, cw=4.0), t) for d, t in (('fixed', 'grid'), ('vrr', 'follow'))]
    row2 = [cell(m.run(disp=d, dxvk='fifo', vs=True, g=6.0, cr=4.0, cw=cr), t) for d, t in (('fixed', 'grid'), ('vrr', 'follow'))]
    print('%-5.1f ms  render-bound: fixed %-10s VRR %-10s  worker-bound: fixed %-10s VRR %-10s' % (cr, row[0], row[1], row2[0], row2[1]))
print('\nwindows %d, HARD ERRORS %d' % (tot, hard))
