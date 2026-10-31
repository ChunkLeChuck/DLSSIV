"""The arithmetic of rcas_ps.hlsl's ceiling (audit SHADER-L44), modelled in float32 and checked on a million random
neighbourhoods. A model of the FORMULA, not of the GPU: it is what says the generalisation from AMD's literal 1 to a
ceiling P is the right one (numerator P - max, divisor 4*min - 4*P), which a wrong one would fail loudly.

  1. P = 1: the new expression equals the old one bit for bit, on SDR neighbourhoods (every tap in 0..1).
  2. a tap above white, P = 1 (what shipped): the lobe is 0, the neighbourhood is NOT sharpened. The defect.
  3. the same neighbourhoods, P = 2.258 (six times white at gamma 2.2): sharpened, and the output stays inside 0..P
     whenever the taps do, which is RCAS's whole promise ("cannot ring past the signal range").
  4. a tap above P: the lobe is 0 again, as before the change.
  CONTROL: the tempting wrong generalisation (raise the numerator's ceiling, leave the divisor's 4 alone) must FAIL 3."""
import sys
import numpy as np
f32 = np.float32
LIMIT = f32(0.25 - 1.0 / 16.0)
def lobe_and_out(b, d, e, f, h, P, con=f32(1.0), wrong=False):
    mn4 = np.minimum(np.minimum(b, d), np.minimum(f, h)); mx4 = np.maximum(np.maximum(b, d), np.maximum(f, h))
    hitMin = np.minimum(mn4, e) / np.maximum(f32(4.0) * mx4, f32(1e-6))
    div = f32(4.0) * mn4 - (f32(4.0) if wrong else f32(4.0) * P)
    hitMax = (P - np.maximum(mx4, e)) / np.minimum(div, f32(-1e-6))
    lobeRGB = np.maximum(-hitMin, hitMax)
    lobe = np.maximum(-LIMIT, np.minimum(lobeRGB.max(axis=1), f32(0.0))) * con
    rcpL = f32(1.0) / (f32(4.0) * lobe + f32(1.0))
    out = (lobe[:, None] * (b + d + f + h) + e) * rcpL[:, None]
    return lobe, out
def old_lobe(b, d, e, f, h):
    mn4 = np.minimum(np.minimum(b, d), np.minimum(f, h)); mx4 = np.maximum(np.maximum(b, d), np.maximum(f, h))
    hitMin = np.minimum(mn4, e) / np.maximum(f32(4.0) * mx4, f32(1e-6))
    hitMax = (f32(1.0) - np.maximum(mx4, e)) / np.minimum(f32(4.0) * mn4 - f32(4.0), f32(-1e-6))
    return np.maximum(-LIMIT, np.minimum(np.maximum(-hitMin, hitMax).max(axis=1), f32(0.0)))
rng = np.random.default_rng(44); N = 1_000_000
taps = lambda hi: [rng.random((N, 3), dtype=np.float32) * f32(hi) for _ in range(5)]
ok = True
# 1
b, d, e, f, h = taps(1.0)
l_new, _ = lobe_and_out(b, d, e, f, h, f32(1.0)); l_old = old_lobe(b, d, e, f, h)
same = int((l_new == l_old).sum()); print("1. P = 1 on %d SDR neighbourhoods: %d identical to the shipped expression, bit for bit" % (N, same)); ok &= same == N
# 2 and 3: neighbourhoods with at least one tap above white, all inside P
P = f32(6.0 ** (1.0 / 2.2))
b, d, e, f, h = taps(float(P)); e = np.maximum(e, f32(1.05))
l1, _ = lobe_and_out(b, d, e, f, h, f32(1.0))
print("2. a tap above white with P = 1: %d of %d neighbourhoods get a lobe of exactly 0 (not sharpened)" % (int((l1 == 0).sum()), N)); ok &= int((l1 == 0).sum()) == N
lP, out = lobe_and_out(b, d, e, f, h, P)
sharp = int((lP < 0).sum()); eps = f32(2e-5)
inside = int(((out >= -eps) & (out <= P + eps)).all(axis=1).sum())
print("3. the same with P = %.3f: %d sharpened, %d of %d outputs inside 0..P (worst overshoot %.2e)" % (P, sharp, inside, N, float(np.maximum(out - P, -out).max())))
ok &= sharp > N // 2 and inside == N
# 4
b, d, e, f, h = taps(float(P)); e = e + P
l4, _ = lobe_and_out(b, d, e, f, h, P)
print("4. a tap above P: %d of %d lobes are 0" % (int((l4 == 0).sum()), N)); ok &= int((l4 == 0).sum()) == N
# control
b, d, e, f, h = taps(float(P)); e = np.maximum(e, f32(1.05))
_, outw = lobe_and_out(b, d, e, f, h, P, wrong=True)
bad = N - int(((outw >= -eps) & (outw <= P + eps)).all(axis=1).sum())
print("CONTROL, the divisor left at 4: %d outputs leave 0..P (worst %.3f above)  -> the control %s" % (bad, float((outw - P).max()), "fails as it should" if bad > 0 else "DID NOT FAIL"))
ok &= bad > 0
print("RCAS CEILING " + ("PASS" if ok else "FAIL")); sys.exit(0 if ok else 1)
