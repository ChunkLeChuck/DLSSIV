// THE POLICY'S ROWS AT THE CONDITIONS' BENCH (round 6, G). Reads policy-rows.txt (beside this file: written from
// docs/FG-POLICY-AND-PLAN-1.1.md and the plan, never from the code) and asks source/fg/fg_cond.h, included UNCHANGED (by /I: the
// tree's, or run.py's planted copy for the control), each row's session: FgCondFirst must be the row's answer.
// A PENDING row (another builder's work this round) does not fail while it does not hold, and fails the day it holds until its mark is
// taken off. Prints a line a row and "POLICY: n rows, a held, p pending, f failed"; exit 0 where none failed, 1 where one did, 2 where
// the file cannot be read.
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include "fg_cond.h"

static std::string Trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == std::string::npos ? std::string() : s.substr(a, b - a + 1);
}
// a value: a rate in frames a second (tenths kept: 59.9 -> 599 where tenths=1), else a whole number
static long Val(const std::string& facts, const char* key, long d, bool tenths)
{
    const std::string k = std::string(" ") + key + "=";
    const std::string f = " " + facts;
    const size_t at = f.find(k);
    if (at == std::string::npos) return d;
    const double v = atof(f.c_str() + at + k.size());
    return tenths ? (long)(v * 10.0 + (v >= 0 ? 0.5 : -0.5)) : (long)v;
}
static bool Has(const std::string& facts, const char* key) { return (" " + facts).find(std::string(" ") + key + "=") != std::string::npos; }

int main(int argc, char** argv)
{
    if (argc < 2) { printf("usage: policy <policy-rows.txt> [noover]\n"); return 2; }
    // (round 7, G) "noover": THE OVERRUN ROWS' CONTROL, the adapter hands the overrun's reading no L (as if it never read one): a row that
    // stands down by its bulk alone must then fail, or the rows could not see an overrun that is never read. No copy of fg_cond.h is
    // planted, so the control does not hang on the reading's own lines, which F's mend changes
    const bool noOver = argc > 2 && !strcmp(argv[2], "noover");
    FILE* f = fopen(argv[1], "r");
    if (!f) { printf("POLICY: cannot open %s\n", argv[1]); return 2; }
    char line[2048];
    int n = 0, held = 0, pending = 0, failed = 0, bad = 0, unasked = 0;
    while (fgets(line, sizeof(line), f))
    {
        const std::string l = Trim(line);
        if (l.empty() || l[0] == '#') continue;
        const size_t p1 = l.find('|'), p2 = p1 == std::string::npos ? p1 : l.find('|', p1 + 1);
        if (p2 == std::string::npos) { printf("  UNREAD: %s\n", l.c_str()); bad++; continue; }
        const size_t p3 = l.find('|', p2 + 1);
        const std::string name = Trim(l.substr(0, p1)), facts = Trim(l.substr(p1 + 1, p2 - p1 - 1));
        const std::string want = Trim(l.substr(p2 + 1, p3 == std::string::npos ? std::string::npos : p3 - p2 - 1));
        const std::string mark = p3 == std::string::npos ? std::string() : Trim(l.substr(p3 + 1));
        // THE SHIPPED BUILD OF THE ROUND 6 DESIGN, then the row's facts
        FgCondIn in = {};
        in.presentOurs = 1; in.markerOk = 1; in.dxvk31 = -1; in.windowed = 1; in.swapEffect = 1; in.backBuffers = 2; in.lockPm = 1015;
        in.bbFp16 = 1; in.hdrOn = 1; in.vsync = 1; in.fixedKey = 1; in.flipQuant = 0;
        if (Has(facts, "num")) { in.refreshNum = (int)Val(facts, "num", 0, false); in.refreshDen = (int)Val(facts, "den", 1, false); }
        else { in.refreshNum = (int)Val(facts, "hz", 0, false); in.refreshDen = 1; }
        in.vrr = (int)Val(facts, "vrr", -1, false);
        in.vsync = (int)Val(facts, "vsync", 1, false);
        if (Val(facts, "sdr", 0, false)) { in.bbFp16 = 0; in.hdrOn = 0; }
        in.engaged = (int)Val(facts, "engaged", 0, false);
        in.realFpsX10 = (int)Val(facts, "game", 0, true);
        in.freeFpsX10 = in.engaged ? (int)Val(facts, "free", 0, true) : 0;
        in.predFpsX10 = in.engaged ? 0 : (int)Val(facts, "pred", in.realFpsX10, true);   // (absent: the game's own)
        (void)Val(facts, "fill", 0, false); (void)Val(facts, "can", 0, false);   // (the fill's facts: read and not handed in since round 8's deep strip; the table has no fill)
        in.menu = (int)Val(facts, "menu", 0, false); in.fixOver = (int)Val(facts, "fixover", 0, false);
        FgCondK k = FgCondDefaults();
        FgCondFloorKey(k, (int)Val(facts, "minfps", 0, false));
        // (round 7, G) THE OVERRUN'S ADAPTER, THE ONE CODE-SHAPED PART OF THIS BENCH: the row's facts are the policy's (the law's L, the
        // share of the game's frames over the period the mode holds it at, the windows engaged with that reading); here they are handed
        // to the reading the code has. (Round 7, on F's merge: round 6's FgCondFixOverStep on L is gone.) F's reading: THE SHARE of the law's
        // last 12 s over two refreshes and FG_COND_FIX_OVER_US (FgCondFixShareStep: over 100 of 1000, with the law's E, the 95th of the last
        // 4 s, still over that line), two windows running; and THE HELD RATE'S DIP (FgCondFixDipStep), two windows running. A row is a steady
        // game for its windows, so the share is over2 (percent, as thousandths) and E is over the line exactly where more than 5 of 100 of the
        // frames are (the 95th); both stepped once a window, wins windows, as FgpFixPublish steps them
        if (Has(facts, "L"))
        {
            int run = 0, dipRun = 0, over = 0;
            const int wins = (int)Val(facts, "wins", 10, false), lineUs = FgCondFloorUs(in) + FG_COND_FIX_OVER_US;
            const int sharePm = noOver ? 0 : (int)Val(facts, "over2", 0, false) * 10;
            const int eUs = sharePm > 50 ? lineUs + 1 : lineUs - 1;
            const int engFixed = (in.engaged && FgCondFixedMode(in)) ? 1 : 0;
            for (int w = 0; w < wins; w++)
            {
                over = FgCondFixShareStep(run, engFixed ? sharePm : -1, engFixed ? eUs : 0, lineUs, engFixed);
                in.fixOver = over; in.fixSharePm = engFixed ? sharePm : -1; in.fixLUs = (int)Val(facts, "L", 0, false);
                in.fixDipFirst = FgCondFixDipStep(dipRun, in);
            }
        }
        // (round 7, G) THE FILL'S CHOICE (answers FILL or DOUBLE, the fill's trade line): not the table's. The fill's step (FgFillStep) is
        // the round's work of F (F3 rewrites its inputs), so this bench asks it NOT YET: such a row is UNASKED, counted and named on the
        // verdict line, and must carry a PENDING mark (an unasked row without one fails). Its adapter is written on F's merge.
        if (want == "FILL" || want == "DOUBLE")
        {
            n++; pending++; unasked++;
            if (mark.empty()) { failed++; printf("  FAIL (AN UNASKED ROW WITHOUT A PENDING MARK): %s\n", name.c_str()); }
            else printf("  UNASKED (%s): %s: the policy %s, the fill's step not asked by this bench yet\n", mark.c_str(), name.c_str(), want.c_str());
            continue;
        }
        const int got = FgCondFirst(in, k);
        const std::string gotName = got == FGC_OK ? "OK" : FgCondEnumName(got);
        const bool ok = gotName == want;
        n++;
        if (!mark.empty())
        {
            pending++;
            if (ok) { failed++; printf("  FAIL (A PENDING ROW NOW HOLDS: take its mark off): %s: %s\n", name.c_str(), gotName.c_str()); }
            else printf("  pending (%s): %s: the table says %s, the policy %s\n", mark.c_str(), name.c_str(), gotName.c_str(), want.c_str());
        }
        else if (ok) { held++; printf("  holds: %s: %s\n", name.c_str(), gotName.c_str()); }
        else { failed++; printf("  FAIL: %s: the table says %s, the policy %s\n", name.c_str(), gotName.c_str(), want.c_str()); }
    }
    fclose(f);
    printf("POLICY: %d rows, %d held, %d pending, %d failed%s; %d of the pending unasked (the fill's choice)\n", n, held, pending, failed + bad, bad ? " (some unread)" : "", unasked);
    return (failed || bad) ? 1 : 0;
}
