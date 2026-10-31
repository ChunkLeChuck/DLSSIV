// THE HELPER'S BUFFERED PIPE READER, PROVED ON A REAL PIPE (the optimisation pass of 2026-09-24, HL-1).
//
// host/pipe_reader.h is included as it ships, and fed through a byte-mode named pipe made with the same flags as the
// helper's Serve(). The framing below is the serve loop's: one tag byte, then the body that tag names, and a tag it
// does not know ends the session. The structs are feed_ipc.h's own, so a size change there changes this test too.
//
// What each case proves:
//   1  one frame per write, in lockstep as the game runs: ONE ReadFile a frame (the saving), every field intact
//   1c the control, the old tag-then-body reads on the same stream: TWO a frame, so the counter above measures reads
//   2  tag and body in two writes 200 ms apart: the body still arrives whole (a partial read, then a second)
//   3  B, F, P, M and F in one write: all five, in order, with their fields, from one ReadFile
//   4  1000 frames in a burst (33 bytes each, so messages straddle the 1024-byte buffer's edge): all 1000 in order
//   5  the game closes in the middle of a body: the read fails, as ReadFull's did, and nothing is invented
//   6  a bad tag between good frames: the good ones before it are served, the loop stops AT it (tag 0x5A), and the
//      frame written after it is never served, exactly as the helper's loop ends the session today
// A watchdog ends the run with exit 3 after 20 s, so a read that waits for bytes that are not coming fails the test
// instead of hanging it.
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../../source/transport/feed_ipc.h"
#include "../../host/pipe_reader.h"

static_assert(sizeof(((HostPipeReader*)0)->buf) == FEED_PIPE_BYTES, "the reader's buffer is the pipe's own in-buffer");

static int g_fail = 0;
#define CHECK(c, ...) do { if (!(c)) { g_fail++; printf("  FAIL line %d: ", __LINE__); printf(__VA_ARGS__); printf("\n"); } } while (0)

struct Pipe { HANDLE srv, cli; };
static Pipe MakePipe(int k)
{
    char name[96]; _snprintf_s(name, sizeof(name), _TRUNCATE, "\\\\.\\pipe\\dlssiv-test-reader.%lu.%d", GetCurrentProcessId(), k);
    Pipe p;
    p.srv = CreateNamedPipeA(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_FIRST_PIPE_INSTANCE,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                             1, FEED_PIPE_BYTES, FEED_PIPE_BYTES, 0, nullptr);
    p.cli = CreateFileA(name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (p.srv == INVALID_HANDLE_VALUE || p.cli == INVALID_HANDLE_VALUE) { printf("could not make pipe %d (%lu)\n", k, GetLastError()); ExitProcess(2); }
    ConnectNamedPipe(p.srv, nullptr);   // already connected: ERROR_PIPE_CONNECTED
    return p;
}
static void Close(Pipe& p) { if (p.cli) CloseHandle(p.cli); CloseHandle(p.srv); p.cli = nullptr; }
static void Put(HANDLE h, const std::vector<BYTE>& v)
{
    DWORD put = 0;
    if (!WriteFile(h, v.data(), (DWORD)v.size(), &put, nullptr) || put != v.size()) { printf("write failed (%lu)\n", GetLastError()); ExitProcess(2); }
}
static void AddFrame(std::vector<BYTE>& v, uint64_t n)
{
    FeedTaggedFrameIvt m = {}; m.tag = 'F'; m.fm.n = n; m.fm.reset = (uint32_t)(n & 1); m.fm.jitter_x = (float)n * 0.25f; m.fm.fov_y_rad = 1.0f;
    const BYTE* b = (const BYTE*)&m; v.insert(v.end(), b, b + sizeof(m));
}

// THE SERVE LOOP'S FRAMING, message by message: 0 served, 1 the pipe ended, 2 an unknown tag (the session ends).
struct Served { BYTE tag; uint64_t n; uint32_t extra; float jx; };
static int ServeOne(HostPipeReader* rd, Served* s)
{
    BYTE tag = 0;
    if (!HostPipeRead(rd, &tag, 1)) return 1;
    s->tag = tag; s->n = 0; s->extra = 0; s->jx = 0.0f;
    if (tag == 'B')      { FeedBuild b = {};     if (!HostPipeRead(rd, &b, sizeof(b))) return 1; s->n = b.sem_in; s->extra = b.width; }
    else if (tag == 'F') { FeedFrameMsg fm = {}; if (!HostPipeRead(rd, &fm, sizeof(fm))) return 1; s->n = fm.n; s->extra = fm.reset; s->jx = fm.jitter_x; }
    else if (tag == 'P') { FeedProbeIvt p = { 'P', 0 }; if (!HostPipeRead(rd, ((BYTE*)&p) + 1, sizeof(p) - 1)) return 1; s->n = p.n; }
    else if (tag == 'M') { FeedModeIvt m = { 'M', 0 }; if (!HostPipeRead(rd, ((BYTE*)&m) + 1, sizeof(m) - 1)) return 1; s->extra = m.mode; }
    else return 2;
    return 0;
}
static bool FrameOk(const Served& s, uint64_t n) { return s.tag == 'F' && s.n == n && s.extra == (uint32_t)(n & 1) && s.jx == (float)n * 0.25f; }

struct Lockstep { HANDLE cli, ack; int frames; };
static DWORD WINAPI LockstepWriter(void* p)
{
    Lockstep* l = (Lockstep*)p;
    for (int i = 1; i <= l->frames; i++) { std::vector<BYTE> v; AddFrame(v, (uint64_t)i); Put(l->cli, v); WaitForSingleObject(l->ack, 5000); }
    return 0;
}
struct Burst { HANDLE cli; std::vector<BYTE> bytes; DWORD gapMs; size_t splitAt; };
static DWORD WINAPI BurstWriter(void* p)
{
    Burst* b = (Burst*)p;
    if (b->splitAt && b->splitAt < b->bytes.size())
    {
        Put(b->cli, std::vector<BYTE>(b->bytes.begin(), b->bytes.begin() + b->splitAt)); Sleep(b->gapMs);
        Put(b->cli, std::vector<BYTE>(b->bytes.begin() + b->splitAt, b->bytes.end()));
    }
    else Put(b->cli, b->bytes);
    return 0;
}
static DWORD WINAPI Watchdog(void*) { Sleep(20000); printf("FAIL: the test took over 20 s: a read waited for bytes that were not coming\n"); fflush(stdout); TerminateProcess(GetCurrentProcess(), 3); return 3; }

int main()
{
    if (HANDLE t = CreateThread(nullptr, 0, Watchdog, nullptr, 0, nullptr)) CloseHandle(t);

    { // 1: lockstep, one frame per write
        Pipe p = MakePipe(1); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        Lockstep l = { p.cli, CreateEventA(nullptr, FALSE, FALSE, nullptr), 200 };
        HANDLE t = CreateThread(nullptr, 0, LockstepWriter, &l, 0, nullptr);
        int ok = 0;
        for (int i = 1; i <= l.frames; i++) { Served s; const int r = ServeOne(&rd, &s); if (r == 0 && FrameOk(s, (uint64_t)i)) ok++; SetEvent(l.ack); }
        WaitForSingleObject(t, 5000); CloseHandle(t);
        CHECK(ok == l.frames, "case 1: %d of %d frames intact", ok, l.frames);
        CHECK(rd.fills == (DWORD)l.frames, "case 1: %lu reads for %d frames, want one a frame", rd.fills, l.frames);
        printf("case 1  lockstep:        %d frames, %lu reads (%.2f a frame)\n", ok, rd.fills, (double)rd.fills / l.frames);
        Close(p);
        // 1c: the control, the old shape (tag, then body, each by its own ReadFile loop) on the same traffic
        Pipe q = MakePipe(11); DWORD oldReads = 0; int okOld = 0;
        Lockstep l2 = { q.cli, l.ack, 200 };
        HANDLE t2 = CreateThread(nullptr, 0, LockstepWriter, &l2, 0, nullptr);
        for (int i = 1; i <= l2.frames; i++)
        {
            BYTE tag = 0; FeedFrameMsg fm = {}; DWORD got = 0;
            BYTE* dst[2] = { &tag, (BYTE*)&fm }; DWORD len[2] = { 1, sizeof(fm) };
            for (int k = 0; k < 2; k++) { DWORD left = len[k]; BYTE* d = dst[k]; while (left) { if (!ReadFile(q.srv, d, left, &got, nullptr) || !got) break; oldReads++; d += got; left -= got; } }
            if (tag == 'F' && fm.n == (uint64_t)i) okOld++;
            SetEvent(l.ack);
        }
        WaitForSingleObject(t2, 5000); CloseHandle(t2); CloseHandle(l.ack);
        CHECK(okOld == l2.frames && oldReads == 2u * l2.frames, "case 1c: the old shape read %lu times for %d frames, want two a frame", oldReads, okOld);
        printf("case 1c control (old):   %d frames, %lu reads (%.2f a frame)\n", okOld, oldReads, (double)oldReads / l2.frames);
        Close(q);
    }
    { // 2: tag and body in two writes, 200 ms apart
        Pipe p = MakePipe(2); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        Burst b = { p.cli, {}, 200, 1 }; AddFrame(b.bytes, 77);
        HANDLE t = CreateThread(nullptr, 0, BurstWriter, &b, 0, nullptr);
        Served s; const int r = ServeOne(&rd, &s);
        WaitForSingleObject(t, 5000); CloseHandle(t);
        CHECK(r == 0 && FrameOk(s, 77), "case 2: the split frame came back wrong (r %d, n %llu)", r, (unsigned long long)s.n);
        CHECK(rd.fills == 2, "case 2: %lu reads, want 2 (the tag's, then the body's)", rd.fills);
        printf("case 2  split write:     frame %llu whole, %lu reads\n", (unsigned long long)s.n, rd.fills);
        Close(p);
    }
    { // 3: five messages of four kinds in one write
        Pipe p = MakePipe(3); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        std::vector<BYTE> v;
        { v.push_back('B'); FeedBuild b = {}; b.width = 2560; b.sem_in = 0x1234; const BYTE* q = (const BYTE*)&b; v.insert(v.end(), q, q + sizeof(b)); }
        AddFrame(v, 5);
        { FeedProbeIvt pr = { 'P', 99 }; const BYTE* q = (const BYTE*)&pr; v.insert(v.end(), q, q + sizeof(pr)); }
        { FeedModeIvt m = { 'M', 2 }; const BYTE* q = (const BYTE*)&m; v.insert(v.end(), q, q + sizeof(m)); }
        AddFrame(v, 6);
        Put(p.cli, v);
        Served s[5]; int r[5];
        for (int i = 0; i < 5; i++) r[i] = ServeOne(&rd, &s[i]);
        CHECK(r[0] == 0 && s[0].tag == 'B' && s[0].extra == 2560 && s[0].n == 0x1234, "case 3: the build");
        CHECK(r[1] == 0 && FrameOk(s[1], 5), "case 3: the first frame");
        CHECK(r[2] == 0 && s[2].tag == 'P' && s[2].n == 99, "case 3: the probe");
        CHECK(r[3] == 0 && s[3].tag == 'M' && s[3].extra == 2, "case 3: the mode");
        CHECK(r[4] == 0 && FrameOk(s[4], 6), "case 3: the second frame");
        CHECK(rd.fills == 1, "case 3: %lu reads for one write of %zu bytes, want 1", rd.fills, v.size());
        printf("case 3  one write, B F P M F: %zu bytes, %lu read\n", v.size(), rd.fills);
        Close(p);
    }
    { // 4: 1000 frames in a burst, across the buffer's edge
        Pipe p = MakePipe(4); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        Burst b = { p.cli, {}, 0, 0 }; for (int i = 1; i <= 1000; i++) AddFrame(b.bytes, (uint64_t)i);
        HANDLE t = CreateThread(nullptr, 0, BurstWriter, &b, 0, nullptr);
        int ok = 0;
        for (int i = 1; i <= 1000; i++) { Served s; if (ServeOne(&rd, &s) == 0 && FrameOk(s, (uint64_t)i)) ok++; }
        WaitForSingleObject(t, 5000); CloseHandle(t);
        CHECK(ok == 1000, "case 4: %d of 1000 frames intact and in order", ok);
        CHECK(rd.fills >= 33 && rd.fills < 1000, "case 4: %lu reads for 33,000 bytes through a 1024-byte buffer", rd.fills);
        printf("case 4  burst:           %d of 1000 in order, %lu reads\n", ok, rd.fills);
        Close(p);
    }
    { // 5: the game closes in the middle of a body
        Pipe p = MakePipe(5); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        std::vector<BYTE> v; AddFrame(v, 1); AddFrame(v, 2); v.resize(v.size() - (sizeof(FeedTaggedFrameIvt) - 11));   // frame 2 cut to its tag and 10 bytes
        Put(p.cli, v); CloseHandle(p.cli); p.cli = nullptr;
        Served s1, s2; const int r1 = ServeOne(&rd, &s1), r2 = ServeOne(&rd, &s2);
        Served s3; const int r3 = ServeOne(&rd, &s3);
        CHECK(r1 == 0 && FrameOk(s1, 1), "case 5: the whole frame before the cut");
        CHECK(r2 == 1, "case 5: the cut frame must end the session (r %d)", r2);
        CHECK(r3 == 1, "case 5: and the pipe stays ended (r %d)", r3);
        printf("case 5  close mid-body:  frame 1 served, the cut frame ends it (%s)\n", r2 == 1 ? "as ReadFull did" : "WRONG");
        Close(p);
    }
    { // 6: a bad tag between good frames
        Pipe p = MakePipe(6); HostPipeReader rd; HostPipeReaderInit(&rd, p.srv);
        std::vector<BYTE> v; AddFrame(v, 1); AddFrame(v, 2); v.push_back('Z'); AddFrame(v, 3);
        Put(p.cli, v);
        int served = 0, r = 0; Served s = {};
        while ((r = ServeOne(&rd, &s)) == 0) { if (FrameOk(s, (uint64_t)(served + 1))) served++; else break; }
        CHECK(served == 2, "case 6: %d good frames before the bad tag, want 2", served);
        CHECK(r == 2 && s.tag == 'Z', "case 6: the loop must stop at the bad tag (r %d, tag 0x%02X)", r, s.tag);
        printf("case 6  bad tag:         %d frames served, stopped at tag 0x%02X, frame 3 never served\n", served, s.tag);
        Close(p);
    }
    if (g_fail) { printf("FAIL: %d check%s\n", g_fail, g_fail == 1 ? "" : "s"); return 1; }
    printf("PASS: the buffered reader keeps the serve loop's framing and reads once a frame\n");
    return 0;
}
