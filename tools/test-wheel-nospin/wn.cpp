// tools\test-wheel-nospin\wn.cpp (2026-10-11): VelWheelNoSpin, the mod's own text (wn_cut.h, cut from source\DLSS-IV.cpp
// by prove.py), against wheels made here in double precision. No Direct3D.
//
// A wheel here is what the game hands the mod: the tyre's matrix M times the car's matrix G, row vectors, where
//   M = spin(about the axle) x steer(about an axis square to the axle) x mirror x place(the hub in the car)
// and G is the car (a turn, a lean, a pitch, a place about 1745 units out, as the game's are). Last frame and this
// frame differ in all of it. WHAT IS WANTED is the tyre as it is turned NOW under everything else as it was LAST FRAME:
//   want = spin(now) x steer(then) x mirror x place(then) x G(then)
// and what the function is given is this frame's matrix and last frame's, nothing else.
// THE CONTROL IN EVERY CASE: last frame's matrix as it came (the vectors so far) must be FAR from want when the wheel
// turned, or the case proves nothing.
#include <cmath>
#include <cstdio>
#include <cstring>
typedef long LONG;
#include "wn_cut.h"

struct M4 { double m[4][4]; };
static M4 Ident() { M4 r; memset(&r, 0, sizeof(r)); for (int i = 0; i < 4; i++) r.m[i][i] = 1; return r; }
static M4 Mul(const M4& a, const M4& b) { M4 r; for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) { double s = 0; for (int k = 0; k < 4; k++) s += a.m[i][k] * b.m[k][j]; r.m[i][j] = s; } return r; }
static M4 Rot(int axis, double deg)
{
    const double a = deg * 3.14159265358979323846 / 180.0, c = cos(a), s = sin(a);
    M4 r = Ident(); const int i = (axis + 1) % 3, j = (axis + 2) % 3;
    r.m[i][i] = c; r.m[i][j] = s; r.m[j][i] = -s; r.m[j][j] = c; return r;
}
static M4 Place(double x, double y, double z) { M4 r = Ident(); r.m[3][0] = x; r.m[3][1] = y; r.m[3][2] = z; return r; }
static M4 Mirror(int axis, bool on) { M4 r = Ident(); if (on) r.m[axis][axis] = -1; return r; }
static void ToF(const M4& a, float* f) { for (int i = 0; i < 4; i++) for (int j = 0; j < 4; j++) f[i * 4 + j] = (float)a.m[i][j]; }

struct Car { double yaw, lean, pitch, x, y, z; };
struct Wheel { double spin, steer, hx, hy, hz; };
static int steerAxisOf(int axle) { return axle == 2 ? 0 : 2; }
static int leanAxisOf(int axle) { return axle == 1 ? 0 : 1; }         // square to the axle: the car's own roll
static M4 Tyre(int axle, bool mirror, const Wheel& w, const Car& c)
{
    // the car: a pitch about the AXLE's own direction (the one turn the function cannot tell from spin), a lean, a turn, a place
    const M4 G = Mul(Mul(Mul(Rot(axle, c.pitch), Rot(leanAxisOf(axle), c.lean)), Rot(steerAxisOf(axle), c.yaw)), Place(c.x, c.y, c.z));
    const M4 M = Mul(Mul(Mul(Rot(axle, w.spin), Rot(steerAxisOf(axle), w.steer)), Mirror(axle, mirror)), Place(w.hx, w.hy, w.hz));
    return Mul(M, G);
}
// the largest difference in the three rows, as an angle-sized number (the rows are unit long), and in the hub's place
static double RowsApart(const float* a, const float* b) { double e = 0; for (int i = 0; i < 12; i++) if (i % 4 != 3) { const double d = fabs((double)a[i] - (double)b[i]); if (d > e) e = d; } return e; }
static double HubApart(const float* a, const float* b) { double e = 0; for (int i = 12; i < 15; i++) { const double d = fabs((double)a[i] - (double)b[i]); if (d > e) e = d; } return e; }

static int fails = 0;
static void Check(bool ok, const char* what) { printf("  %s  %s\n", ok ? "ok  " : "FAIL", what); if (!ok) fails++; }

int main()
{
    const double spins[] = { 45.0, -45.0, 10.0, 120.0, 180.0, 200.0, 3.5 };
    double worst = 0, worstRaw = 1e9, worstHub = 0; int wrongAxle = 0, n = 0;
    for (int axle = 0; axle < 3; axle++)
        for (int mirror = 0; mirror < 2; mirror++)
        {
            volatile LONG seen = -1;         // a session's memory: the first spin of the list is a clear one
            for (double dspin : spins)
                for (int motion = 0; motion < 3; motion++)
                {
                    // last frame and this one: the car half a unit on, turning, leaning; the wheel steered and sprung
                    const Car   cp = { 31.0, 1.0, 0.0, 1745.25, -820.5, 14.0 };
                    const Car   cc = { 31.0 + (motion ? 2.0 : 0.0), 1.0 + (motion == 2 ? 0.5 : 0.0), 0.0, 1745.25 + 0.41, -820.5 + 0.29, 14.0 + (motion == 2 ? 0.02 : 0.0) };
                    const Wheel wp = { 77.0, 4.0, 0.8, 1.3, -0.25 };
                    const Wheel wc = { 77.0 + dspin, 4.0 + (motion ? 3.0 : 0.0), 0.8, 1.3, -0.25 + (motion == 2 ? 0.02 : 0.0) };
                    const Wheel ww = { wc.spin, wp.steer, wp.hx, wp.hy, wp.hz };
                    float cur[16], prev[16], want[16], raw[16];
                    ToF(Tyre(axle, mirror != 0, wc, cc), cur); ToF(Tyre(axle, mirror != 0, wp, cp), prev); ToF(Tyre(axle, mirror != 0, ww, cp), want);
                    memcpy(raw, prev, sizeof(raw));
                    const int a = VelWheelNoSpin(cur, prev, &seen);
                    const double e = RowsApart(prev, want), er = RowsApart(raw, want), eh = HubApart(prev, want);
                    if (a != axle) wrongAxle++;
                    if (e > worst) worst = e;
                    if (er < worstRaw) worstRaw = er;
                    if (eh > worstHub) worstHub = eh;
                    n++;
                }
        }
    printf("  %d wheels (three axles, mirrored and not, seven spins, the car straight, turning, turning and leaning):\n", n);
    printf("  the rows end %.5f from the wanted ones at worst; as they came they were %.5f away at best; the hub %.6f\n", worst, worstRaw, worstHub);
    Check(wrongAxle == 0, "the axle is found every time, whichever of the mesh's axes it is");
    Check(worst < 2.0e-3, "the tyre comes out as it is turned now, placed as it was: within 0.002 (a ninth of a degree) with the car turning, leaning and steering");
    Check(worstRaw > 0.05, "CONTROL: last frame's matrix as it came is at least 0.05 from that in every case (the cases can tell)");
    Check(worstHub == 0.0, "the hub's place is last frame's, bit for bit");
    {   // what it cannot see: the car pitching about the axle. One degree of it is lost with the spin, and no more
        const Car cp = { 31.0, 1.0, 0.0, 1745.25, -820.5, 14.0 }, cc = { 33.0, 1.0, 1.0, 1745.66, -820.21, 14.0 };
        const Wheel wp = { 77.0, 4.0, 0.8, 1.3, -0.25 }, wc = { 122.0, 4.0, 0.8, 1.3, -0.25 };
        float cur[16], prev[16], want[16];
        ToF(Tyre(0, false, wc, cc), cur); ToF(Tyre(0, false, wp, cp), prev); ToF(Tyre(0, false, { wc.spin, wp.steer, wp.hx, wp.hy, wp.hz }, cp), want);
        volatile LONG seen = -1;
        const int a = VelWheelNoSpin(cur, prev, &seen); const double e = RowsApart(prev, want);
        printf("  the car pitching one degree about the axle in the frame: the rows end %.5f from the wanted ones\n", e);
        Check(a == 0 && e > 0.010 && e < 0.020, "a pitch of the car about the axle is taken for spin: one degree of it leaves 0.0175, as the comment says, and no more");
    }
    {   // THE CREEPING CAR: a spin of 3.5 degrees while the car turns two and the wheel steers three. The steering axis's row is
        // the stillest then. With the axle remembered the answer is right; with nothing remembered it is off by those small turns
        const Car cp = { 31.0, 1.0, 0.0, 1745.25, -820.5, 14.0 }, cc = { 33.0, 1.0, 0.0, 1745.66, -820.21, 14.0 };
        const Wheel wp = { 77.0, 4.0, 0.8, 1.3, -0.25 }, wc = { 80.5, 7.0, 0.8, 1.3, -0.25 };
        float cur[16], prev[16], want[16], keep[16];
        ToF(Tyre(0, false, wc, cc), cur); ToF(Tyre(0, false, wp, cp), prev); ToF(Tyre(0, false, { wc.spin, wp.steer, wp.hx, wp.hy, wp.hz }, cp), want);
        memcpy(keep, prev, sizeof(keep));
        volatile LONG none = -1, known = 0;
        const int a0 = VelWheelNoSpin(cur, prev, &none); const double e0 = RowsApart(prev, want);
        memcpy(prev, keep, sizeof(keep));
        const int a1 = VelWheelNoSpin(cur, prev, &known); const double e1 = RowsApart(prev, want);
        printf("  the creeping car: with the axle remembered row %d, %.5f from the wanted rows; with none remembered row %d, %.5f\n", a1, e1, a0, e0);
        Check(a1 == 0 && e1 < 2.0e-3 && known == 0, "the creeping car, the axle remembered: right, and the memory is not overwritten by a frame that is not clear");
        Check(a0 != 0 && none == -1 && e0 > 0.02 && e0 < 0.12, "CONTROL: with nothing remembered the stillest row is NOT the axle there (this is what the memory is for), and the error stays under 0.12");
    }
    {   // the answers
        volatile LONG seen = -1;
        const Car c0 = { 31.0, 1.0, 0.0, 1745.25, -820.5, 14.0 };
        const Wheel w0 = { 77.0, 4.0, 0.8, 1.3, -0.25 };
        float cur[16], prev[16], keep[16];
        ToF(Tyre(1, false, { 78.0, 4.0, 0.8, 1.3, -0.25 }, c0), cur); ToF(Tyre(1, false, w0, c0), prev);
        Check(VelWheelNoSpin(cur, prev, &seen) == 4, "a wheel that turned one degree: answer 4 (no turn to see)");
        ToF(Tyre(1, false, w0, c0), cur); ToF(Tyre(1, false, w0, c0), prev); memcpy(keep, prev, sizeof(keep));
        Check(VelWheelNoSpin(cur, prev, &seen) == 4 && RowsApart(prev, keep) < 1e-6, "a wheel at rest: answer 4, and its matrix is as it was");
        ToF(Tyre(1, false, w0, c0), cur); ToF(Tyre(1, false, w0, c0), prev);
        {   // half a turn about the rows' own diagonal n: row -> 2 n (n . row) - row, and every row ends 109 degrees away
            float nrm[3] = { cur[0] + cur[4] + cur[8], cur[1] + cur[5] + cur[9], cur[2] + cur[6] + cur[10] };
            const float l = sqrtf(nrm[0] * nrm[0] + nrm[1] * nrm[1] + nrm[2] * nrm[2]); for (float& v : nrm) v /= l;
            for (int r = 0; r < 3; r++) { const float d = nrm[0] * cur[4 * r] + nrm[1] * cur[4 * r + 1] + nrm[2] * cur[4 * r + 2]; for (int k = 0; k < 3; k++) prev[4 * r + k] = 2.0f * nrm[k] * d - cur[4 * r + k]; }
        }
        memcpy(keep, prev, sizeof(keep));
        Check(VelWheelNoSpin(cur, prev, &seen) == 3 && !memcmp(prev, keep, sizeof(keep)) && seen == -1, "every row turned 109 degrees (another car in the slot): answer 3, last frame's matrix untouched, nothing remembered");
        ToF(Tyre(1, false, w0, c0), cur); ToF(Tyre(1, false, w0, c0), prev); prev[4] = prev[5] = prev[6] = 0.0f; memcpy(keep, prev, sizeof(keep));
        Check(VelWheelNoSpin(cur, prev, &seen) == 3 && !memcmp(prev, keep, sizeof(keep)), "an empty row: answer 3, untouched");
    }
    if (fails) { printf("FAIL: %d check(s)\n", fails); return 1; }
    printf("PASS\n");
    return 0;
}
