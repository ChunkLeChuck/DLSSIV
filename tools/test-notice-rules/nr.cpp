// THE NOTICE'S RULES, PROVEN (2026-10-11). Not shipped. The rules' own text, cut from source\notice.h by prove.py into
// nr_cut.h (from NOTICE RULES START to NOTICE RULES END: the queue, the table that turns a warning's line into a notice,
// the fade, the two copies' event, the cfg override), compiled here with stand-ins for the three things of the mod's it
// touches, and driven two ways:
//     nr.exe warn <lines.txt> <out.txt>     each line of the file is a warning as IvtWarn would have formatted it; for
//                                           each, the notice it makes ("line 1 <tab> line 2 <tab> seconds") or "-"
//     nr.exe rules <out.txt>                the queue's rules, the fade, the did-not-start texts, the cfg override and
//                                           the two copies' event, each as "name=value" for prove.py to judge
// No Direct3D, no window.
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool ntcOff = false;
#define IVT_CFG_COMPLAINTS 8
static char ivtCfgBad[IVT_CFG_COMPLAINTS][96] = {};
static int  ivtCfgBadN = 0, ivtCfgBadSeen = 0;
#include "nr_cut.h"

static void Fresh() { NoticeDropAll(); ntcSeenN = 0; }
static int TakeAll(FILE* o, const char* tag)
{
    NtcItem it; int n = 0;
    while (NoticeTake(&it)) { if (o) fprintf(o, "%s.%d=%s|%s|%d\n", tag, n, it.l1, it.l2, it.seconds); n++; }
    return n;
}

int main(int argc, char** argv)
{
    if (argc == 4 && !strcmp(argv[1], "warn"))
    {
        FILE* in = fopen(argv[2], "rb"); FILE* o = fopen(argv[3], "wb");
        if (!in || !o) return 2;
        static char line[2048];
        while (fgets(line, sizeof(line), in))
        {
            size_t n = strlen(line);
            while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = 0;
            Fresh();
            NoticeFromWarn(line);
            NtcItem it;
            if (NoticeTake(&it)) fprintf(o, "%s\t%s\t%d\n", it.l1, it.l2, it.seconds); else fprintf(o, "-\n");
        }
        fclose(in); fclose(o);
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "rules"))
    {
        FILE* o = fopen(argv[2], "wb"); if (!o) return 2;
        // once a session
        Fresh(); NoticePost("A", "a2", 10, false); NoticePost("A", "a2", 10, false);
        fprintf(o, "once.queued=%d\n", TakeAll(nullptr, ""));
        NoticePost("A", "a2", 10, false);
        fprintf(o, "once.afterShown=%d\n", TakeAll(nullptr, ""));
        // the reply comes each time, and not twice while it waits
        Fresh(); NoticePost("B", "", 10, true); NoticePost("B", "", 10, true);
        fprintf(o, "again.whileWaiting=%d\n", TakeAll(nullptr, ""));
        NoticePost("B", "", 10, true);
        { const int n = TakeAll(nullptr, ""); fprintf(o, "again.later=%d pending=%ld\n", n, ntcPending); }   // the take first: an argument list has no order
        // four, then the rest as one line
        Fresh();
        for (int i = 0; i < 6; i++) { char t[8]; sprintf(t, "N%d", i); NoticePost(t, "x", 10, false); }
        fprintf(o, "more.pendingBefore=%ld\n", ntcPending);
        fprintf(o, "more.taken=%d\n", TakeAll(o, "more"));
        { NtcItem it; const int none = NoticeTake(&it) ? 0 : 1; fprintf(o, "more.thenNothing=%d pending=%ld\n", none, ntcPending); }
        // empty and null texts are not notices; dropping empties the queue
        Fresh(); NoticePost("", "x", 10, false); NoticePost(nullptr, "x", 10, false);
        fprintf(o, "empty.queued=%d\n", TakeAll(nullptr, ""));
        NoticePost("C", "x", 10, false); NoticeDropAll();
        { const int n = TakeAll(nullptr, ""); fprintf(o, "drop.queued=%d pending=%ld\n", n, ntcPending); }
        // the fade
        static const int at[] = { 0, 125, 250, 5000, 9600, 9800, 9999, 10000, 20000 };
        for (int t : at) fprintf(o, "shown.%d=%.4f\n", t, (double)NoticeShown((ULONGLONG)t, 10));
        fprintf(o, "shown15.14800=%.4f\n", (double)NoticeShown(14800, 15));
        // the did-not-start note's three forms
        Fresh(); NoticeFatal(false, false); TakeAll(o, "fatal.start");
        Fresh(); NoticeFatal(true, false);  TakeAll(o, "fatal.stopped");
        Fresh(); NoticeFatal(false, true);  TakeAll(o, "fatal.elsewhere");
        // the cfg override: found among the lines the parser could not use, taken out, and 0 alone turns notices off
        strcpy(ivtCfgBad[0], "sharpnes = \"0.5\" is not a key this build knows");
        strcpy(ivtCfgBad[1], "notice = \"0\" is not a key this build knows");
        strcpy(ivtCfgBad[2], "hdr = \"x\" is not a number");
        ivtCfgBadN = 3; ivtCfgBadSeen = 3; ntcOff = false;
        NoticeCfgOverride();
        fprintf(o, "cfg.zero=off %d n %d seen %d | %s | %s\n", ntcOff ? 1 : 0, ivtCfgBadN, ivtCfgBadSeen, ivtCfgBad[0], ivtCfgBad[1]);
        strcpy(ivtCfgBad[0], "notice = \"1\" is not a key this build knows");
        strcpy(ivtCfgBad[1], "NOTICE = \"00\" is not a key this build knows");
        strcpy(ivtCfgBad[2], "noticed = \"0\" is not a key this build knows");
        ivtCfgBadN = 3; ivtCfgBadSeen = 5; ntcOff = false;
        NoticeCfgOverride();
        fprintf(o, "cfg.other=off %d n %d seen %d | %s\n", ntcOff ? 1 : 0, ivtCfgBadN, ivtCfgBadSeen, ivtCfgBad[0]);
        ivtCfgBadN = 0; ivtCfgBadSeen = 0; ntcOff = false;
        NoticeCfgOverride();
        fprintf(o, "cfg.none=off %d n %d seen %d\n", ntcOff ? 1 : 0, ivtCfgBadN, ivtCfgBadSeen);
        // the two copies: nothing before the second copy speaks, the notice after
        Fresh(); NoticeWatchSecondCopy(); Sleep(150);
        fprintf(o, "twin.before=%d\n", TakeAll(nullptr, ""));
        NoticeTellFirstCopy();
        for (int i = 0; i < 40 && !ntcPending; i++) Sleep(25);
        TakeAll(o, "twin.after");
        fprintf(o, "done=1\n");
        fclose(o);
        return 0;
    }
    return 2;
}
