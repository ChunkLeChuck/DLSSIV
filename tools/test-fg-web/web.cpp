// THE WEB PAGE'S STATE, FOUND IN ROCKSTAR'S OWN EXECUTABLES AT A DESK BY THE MOD'S OWN LINES (2026-10-13; source/fg/fg_web_rule.h, FgWebFindIn;
// source/fg/fg_web.h says what the state is for). No game runs: a GTAIV.exe is read as a file and laid out as Windows
// lays it out (its headers, each section at its virtual address), and the search is handed what the mod hands it in
// the game: the first executable section (DLSS-IV.cpp, TextRange), the menu flag's address, the image's range.
// THE MENU FLAG is found here by the mod's own vote, written again from DLSS-IV.cpp's MenuFlagFind (that function logs
// and reads the running game, so it cannot be compiled here): cmp byte ptr [flag], 0 / jz +0D / call / test al, al,
// the address named most often, at least 4 times and 3 times the runner-up.
// WHAT IS PRINTED, a file: its size; the flag and its votes; the search's answer with its counts; and THREE CONTROLS on a copy
// of the code, each of which must be refused with its own reason: the flag by another address (the shape is
// nowhere); the id's one "mov dword ptr [a], -1" changed to -2 (the writers' count); the byte the frame function
// sets to 1 changed to 0 (the shape is nowhere).
//     web.exe <GTAIV.exe> [<GTAIV.exe> ...]        tools\test-fg-web\run.py builds it and holds the answers
// It reads; it writes nothing and changes nothing. Rockstar's files are not in the repository.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include "../../source/fg/fg_web_rule.h"

static uint32_t U32(const std::vector<unsigned char>& d, size_t o) { uint32_t v = 0; if (o + 4 <= d.size()) memcpy(&v, &d[o], 4); return v; }
static uint16_t U16(const std::vector<unsigned char>& d, size_t o) { uint16_t v = 0; if (o + 2 <= d.size()) memcpy(&v, &d[o], 2); return v; }

static int One(const char* path)
{
    FILE* f = nullptr; fopen_s(&f, path, "rb");
    if (!f) { printf("FILE %s: cannot be opened\n", path); return 1; }
    std::vector<unsigned char> d; unsigned char buf[65536]; size_t got;
    while ((got = fread(buf, 1, sizeof(buf), f)) > 0) d.insert(d.end(), buf, buf + got);
    fclose(f);
    const uint32_t pe = U32(d, 0x3C);
    if (d.size() < 0x400 || U16(d, 0) != 0x5A4D || U32(d, pe) != 0x00004550 || U16(d, pe + 24) != 0x10B) { printf("FILE %s: not a 32-bit PE file\n", path); return 1; }
    const uint16_t nsec = U16(d, pe + 6), optSize = U16(d, pe + 20);
    const uint32_t base = U32(d, pe + 24 + 28), imgSize = U32(d, pe + 24 + 56), hdrSize = U32(d, pe + 24 + 60);
    std::vector<unsigned char> img(imgSize, 0);
    memcpy(img.data(), d.data(), hdrSize < d.size() ? hdrSize : d.size());
    uint32_t tVa = 0, tSize = 0;
    for (int i = 0; i < nsec; i++)
    {
        const size_t o = pe + 24 + optSize + (size_t)i * 40;
        const uint32_t vsize = U32(d, o + 8), va = U32(d, o + 12), rsize = U32(d, o + 16), rptr = U32(d, o + 20), ch = U32(d, o + 36);
        const uint32_t n = rsize < vsize ? rsize : vsize;
        if ((size_t)va + n <= img.size() && (size_t)rptr + n <= d.size()) memcpy(&img[va], &d[rptr], n);
        if (!tSize && (ch & 0x20000000u)) { tVa = va; tSize = vsize; }          // (TextRange: the first executable section, its virtual size)
    }
    if (!tSize || (size_t)tVa + tSize > img.size()) { printf("FILE %s: no executable section\n", path); return 1; }
    const unsigned char* const t = &img[tVa]; const size_t n = tSize;
    // the menu flag, by the mod's vote
    struct Vote { uint32_t a; int n; } v[16] = {}; int vn = 0;
    for (size_t i = 0; i + 16 <= n; i++)
    {
        const unsigned char* p = t + i;
        if (p[0] != 0x80 || p[1] != 0x3D || p[6] != 0x00 || p[7] != 0x74 || p[8] != 0x0D || p[9] != 0xE8 || p[14] != 0x84 || p[15] != 0xC0) continue;
        const uint32_t a = FgWebU32(p + 2);
        if (a < base || a >= base + imgSize) continue;
        int k = 0; while (k < vn && v[k].a != a) k++;
        if (k == vn) { if (vn == 16) continue; v[vn++].a = a; }
        v[k].n++;
    }
    int best = -1, second = 0;
    for (int k = 0; k < vn; k++) if (best < 0 || v[k].n > v[best].n) best = k;
    for (int k = 0; k < vn; k++) if (k != best && v[k].n > second) second = v[k].n;
    const bool flagOk = best >= 0 && v[best].n >= 4 && v[best].n >= 3 * (second > 1 ? second : 1);
    const uint32_t flag = flagOk ? v[best].a : 0;
    printf("FILE %s: %zu bytes; image base 0x%X, %u bytes; the code at +0x%X, %u bytes\n", path, d.size(), base, imgSize, tVa, tSize);
    printf("  MENU FLAG: %s +0x%X (%d sites to %d)\n", flagOk ? "found at" : "NOT found; the best", best >= 0 ? v[best].a - base : 0, best >= 0 ? v[best].n : 0, second);
    FgWebFound w;
    const uint32_t a = FgWebFindIn(t, n, flag, base, base + imgSize, &w);
    printf("  WEB PAGE: %s +0x%X: %s; the shape at %d place(s), %d naming another address; written -1 %d time(s), 0 %d time(s), from eax %d time(s); in the file the dword holds %d\n",
           a ? "found at" : "NOT found; named", w.at ? w.at - base : 0, FgWebWhy(w.why), w.sites, w.others, w.setM1, w.set0, w.setEax, a ? (int)FgWebU32(&img[a - base]) : 0);
    if (!a) return 0;
    // THE CONTROLS, on a copy of the code
    int held = 0;
    {
        FgWebFound c; const uint32_t r = FgWebFindIn(t, n, flag + 1, base, base + imgSize, &c);
        const bool ok = r == 0 && c.why == FG_WEB_NO_SITE; held += ok;
        printf("  CONTROL 1, the flag by another address: %s (%s)\n", ok ? "refused" : "NOT REFUSED", FgWebWhy(c.why));
    }
    std::vector<unsigned char> copy(t, t + n);
    unsigned char ab[4]; memcpy(ab, &a, 4); unsigned char fb[4]; memcpy(fb, &flag, 4);
    {
        size_t at = 0;
        for (size_t i = 2; i + 8 <= n && !at; i++) if (!memcmp(&copy[i], ab, 4) && copy[i - 2] == 0xC7 && copy[i - 1] == 0x05 && FgWebU32(&copy[i + 4]) == 0xFFFFFFFFu) at = i;
        if (at) copy[at + 4] = 0xFE;
        FgWebFound c; const uint32_t r = FgWebFindIn(copy.data(), n, flag, base, base + imgSize, &c);
        const bool ok = at && r == 0 && c.why == FG_WEB_WRITERS && c.setM1 == 0; held += ok;
        printf("  CONTROL 2, the one write of -1 changed to -2 (at +0x%zX): %s (%s; written -1 %d time(s))\n", at ? tVa + at - 2 : 0, ok ? "refused" : "NOT REFUSED", FgWebWhy(c.why), c.setM1);
        if (at) copy[at + 4] = 0xFF;
    }
    {
        size_t at = 0;
        for (size_t i = 9; i + 11 <= n && !at; i++)
            if (!memcmp(&copy[i], fb, 4) && copy[i + 4] == 0xC6 && copy[i + 5] == 0x05 && copy[i + 10] == 0x01 && (copy[i - 1] == 0xA0 || (copy[i - 2] == 0x8A && (copy[i - 1] & 0xC7) == 0x05))
                && (!memcmp(&copy[i - 5], ab, 4) || !memcmp(&copy[i - 7], ab, 4) || !memcmp(&copy[i - 6], ab, 4))) at = i;
        if (at) copy[at + 10] = 0x00;
        FgWebFound c; const uint32_t r = FgWebFindIn(copy.data(), n, flag, base, base + imgSize, &c);
        const bool ok = at && r == 0 && c.why == FG_WEB_NO_SITE; held += ok;
        printf("  CONTROL 3, the byte the frame function sets to 1 changed to 0 (the site at +0x%zX): %s (%s)\n", at ? tVa + at : 0, ok ? "refused" : "NOT REFUSED", FgWebWhy(c.why));
    }
    printf("  CONTROLS HELD: %d of 3\n", held);
    return 0;
}

int main(int argc, char** argv)
{
    if (argc < 2) { printf("web.exe <GTAIV.exe> [<GTAIV.exe> ...]\n"); return 2; }
    int bad = 0;
    for (int i = 1; i < argc; i++) bad += One(argv[i]);
    return bad ? 1 : 0;
}
