// WHAT EACH PASS OF A GENERATED FRAME COSTS ON THE GAME'S OWN CAPTURED FRAMES, THROUGH ANY BUILD OF THE LIBRARY (DLSS-IV,
// 2026-10-12, the library's last round of phase 3; docs\reports\fg-3\sitting-2026-10-12\BRIEF-lib4.md, part 2).
//
// WHY. The cost bench (tools\test-fg-cost, tools\test-fg-own\passes.py) prices a pass on a made picture, at the game's
// clocks, in eight minutes a library. A variant that relights samples costs by WHAT THE PICTURE HOLDS (how many pixels
// take its own path), and a change of it wants its price in seconds. This tool is the replay (tools\fg-replay) with
// the pictures left on the card and the library's own stamps on:
//   - a run's frames are uploaded ONCE, each into textures of its own (16 frames: about 1.5 GB of the card's memory);
//   - the context is created with DLSSIV_FG_CREATE_STAMPS; a frame is one list (prepare, optical flow, interpolate);
//   - the run is driven --loops times from its reset, nothing between two frames but the wait for the list, so the
//     card stays at work and at its clocks;
//   - a pass's number is THE LOWER QUARTILE of its spans over the steady frames (--from, 12: the 12th dispatch after
//     the reset and later) of every loop but the first: other work on the card can only add to a span.
// IT IS NOT THE COST BENCH: the stamps' barriers stretch a list, and the absolute numbers are this tool's own. It
// answers "which of two libraries is cheaper on this run, in which pass, by how much"; the price a pair is the cost
// bench's. No image is read back and nothing is compared: the picture is tools\test-fg-own\lib4.py's.
//
// THE SECOND LAP (--lap-reset=0 --out=<folder> --dump=K): THE PICTURE WITH A LONG HISTORY. A captured run is 16 frames from a reset, and a
// variant that keeps something of the real frames at the screen place (v-env's envelopes) has, at the run's last
// frames, seen 15 of them; in the game it has seen every frame since the last scene cut. With --lap-reset=0 only the
// first lap begins with the run's reset: every later lap goes on from frame 15 to frame 0 with no reset (that one
// pair is a jump and its generated image is nothing), so that the frames --dump and later of THE LAST LAP are made with
// the whole run before them. They are read back and written as the replay writes them (fNN-gen-replay.raw).
//
//     fgtime --run=<a run's folder> --lib=<the library, a full path> [--loops=12] [--from=12] [--csv=<file>]
//            [--lap-reset=1|0] [--out=<folder> --dump=K] [--position=t]
// (2026-10-17) --position=t prices a generated frame at another place than the middle (a library of interface 2, the cut
// fi-position; 0, the default, is the middle and goes as interface 1's description).
//
// (2026-10-18, the true fill's package G1: THE FORMS OF A PAIR, a library of interface 3, the cut fi-store-apart; ffx-own\glue\dlssiv_fg.h)
//            [--form=one|two|two-control] [--t1=0.3333333 --t2=0.6666667] [--bare=<frames>|all] [--bare-control=1]
//            [--open=<frames> [--close=1]] [--reset-at=<frames>] [--reset-interp-at=<frames>] [--stamps=1|0] [--raw=1]
//   --form=two          every pair (but a bare one) makes TWO generated frames: interpolate(t1, NO_STORE), interpolate(t2); the
//                       two images are fNN-gen-a and fNN-gen-b
//   --form=two-control  THE CONTROL: the same with the first interpolate told nothing, so that IT KEEPS the pair's picture and
//                       the second interpolates from the pair's own picture to itself. Its second image must differ
//   --bare=<frames>     those pairs (indices of the run, with commas; "all": every pair) have NO generated frame: prepare,
//                       optical flow, interpolate(STORE_ONLY, no output). --bare-control=1: THE CONTROL, prepare and optical
//                       flow alone (the pair is not kept): the pair after it must differ
//   --open=<frames>     those pairs' one interpolate is told NO_STORE and nothing closes the pair: the next pair's interpolate
//                       must reset (the glue's rule), as one told RESET does (--reset-interp-at: the interpolate alone is told,
//                       optical flow is not, as the glue's own reset of it leaves optical flow). --close=1: an interpolate(STORE_ONLY)
//                       closes each such pair, which is then one generated frame made in two calls
//   --stamps=0          the context without stamps (the product's own path, as the helper creates it); nothing is priced
//   --out=<folder>      with --dump=K: every generated image of the last loop from frame K on is read back and its SHA-256
//                       written to <folder>\hashes.txt ("fNN <tag> <sha256>"); --raw=1 also writes the image (fNN-<tag>.raw)
// Every call's spans together, a call by its place in the list (prepare, optical flow, interpolate #1, interpolate #2),
// are printed beside the passes': THE CALL lines.
//
// It opens no window. It is a GPU user: it is built as fgpipe-time.exe, a name every proof's wait for the card sees.
#define main fgreplay_main
#include "../fg-replay/fgreplay.cpp"
#undef main
#include <algorithm>

struct Key { std::string name; int call = 0; int nth = 0; };
static double Quartile(std::vector<double> v, double q)
{
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const double at = q * (double)(v.size() - 1); const size_t a = (size_t)at; const size_t b = a + 1 < v.size() ? a + 1 : a;
    return v[a] + (v[b] - v[a]) * (at - (double)a);
}
// (2026-10-18) a list of frame indices, "3,7,8"; "all" is every one
static bool InList(const std::string& list, int i)
{
    if (list == "all") return true;
    for (const std::string& s : Split(list, ',')) if (!s.empty() && atoi(s.c_str()) == i) return true;
    return false;
}

int main(int argc, char** argv)
{
    const std::string run = Arg(argc, argv, "--run", ""), libPath = Arg(argc, argv, "--lib", ""), csv = Arg(argc, argv, "--csv", "");
    const int loops = atoi(Arg(argc, argv, "--loops", "12").c_str()), from = atoi(Arg(argc, argv, "--from", "12").c_str());
    const bool lapReset = atoi(Arg(argc, argv, "--lap-reset", "1").c_str()) != 0;
    const std::string outDir = Arg(argc, argv, "--out", ""); const int dumpFrom = atoi(Arg(argc, argv, "--dump", "-1").c_str());
    // (2026-10-18) the forms of a pair (interface 3)
    const std::string form = Arg(argc, argv, "--form", "one"), bare = Arg(argc, argv, "--bare", ""), open = Arg(argc, argv, "--open", ""), resetAt = Arg(argc, argv, "--reset-at", "");
    const float t1 = (float)atof(Arg(argc, argv, "--t1", "0.33333333").c_str()), t2 = (float)atof(Arg(argc, argv, "--t2", "0.66666667").c_str());
    const bool closeOpen = atoi(Arg(argc, argv, "--close", "0").c_str()) != 0;
    const std::string resetInterpAt = Arg(argc, argv, "--reset-interp-at", "");     // those pairs' interpolates alone are told RESET (not the prepare, not optical flow)
    const bool bareControl = atoi(Arg(argc, argv, "--bare-control", "0").c_str()) != 0, stamps = atoi(Arg(argc, argv, "--stamps", "1").c_str()) != 0, raw = atoi(Arg(argc, argv, "--raw", "0").c_str()) != 0;
    const bool two = form == "two" || form == "two-control", twoControl = form == "two-control";
    if (!outDir.empty()) CreateDirectoryA(outDir.c_str(), nullptr);
    if (run.empty() || libPath.empty() || loops < 2) { printf("fgtime --run=<a run's folder> --lib=<the library, a full path> [--loops=12] [--from=12] [--csv=<file>]\nFAIL\n"); return 2; }
    if (!two && form != "one") return Die("--form=%s: one, two or two-control", form.c_str());
    Index ix;
    if (!ParseIndex(run + "\\index.txt", &ix) || !ix.ended) return Die("%s\\index.txt could not be read or the run is not whole", run.c_str());
    const int N = atoi(ix.kv["frames"].c_str());
    const bool hud = ix.kv["hudless"] == "1";
    const FileEnt* fPres = FindFile(ix, 0, "present"); const FileEnt* fGen = FindFile(ix, 0, "gen"); const FileEnt* fDepth = FindFile(ix, 0, "depth"); const FileEnt* fMv = FindFile(ix, 0, "mv");
    if (N < 2 || !fPres || !fGen || !fDepth || !fMv) return Die("the index names no presented, generated, depth or vectors file for frame 0");
    const uint32_t createFlags = (uint32_t)strtoul(ix.kv["create_flags"].c_str(), nullptr, 0);

    std::vector<uint8_t> libBytes;
    if (!ReadAll(libPath, &libBytes)) return Die("the library %s could not be read", libPath.c_str());
    const HMODULE mod = LoadLibraryA(libPath.c_str());
    if (!mod) return Die("the library %s would not load (%lu)", libPath.c_str(), GetLastError());
    const DlssivFgInfoFn fnInfo = (DlssivFgInfoFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_INFO);
    const DlssivFgCreateFn fnCreate = (DlssivFgCreateFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_CREATE);
    const DlssivFgDestroyFn fnDestroy = (DlssivFgDestroyFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_DESTROY);
    const DlssivFgPrepareFn fnPrepare = (DlssivFgPrepareFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_PREPARE);
    const DlssivFgOpticalFlowFn fnFlow = (DlssivFgOpticalFlowFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_OPTICAL_FLOW);
    const DlssivFgInterpolateFn fnInterp = (DlssivFgInterpolateFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_INTERPOLATE);
    const DlssivFgStampsFn fnStamps = (DlssivFgStampsFn)(void*)GetProcAddress(mod, DLSSIV_FG_NAME_STAMPS);
    if (!fnInfo || !fnCreate || !fnDestroy || !fnPrepare || !fnFlow || !fnInterp || !fnStamps) return Die("the library %s lacks a function of the mod's interface", libPath.c_str());
    DlssivFgInfo said = {}; said.size = sizeof(said);
    if (fnInfo(&said, nullptr) != DLSSIV_FG_OK || said.iface < 1u || said.iface > DLSSIV_FG_INTERFACE || !(said.caps & DLSSIV_FG_CAP_STAMPS)) return Die("the library speaks another interface or cannot stamp its passes");
    said.build[sizeof(said.build) - 1] = 0;
    // (2026-10-17, interface 2: ffx-own\glue\dlssiv_fg.h) --position=t: where the generated frame stands, 0 the middle (the default);
    // another value only to a library of interface 2, which holds it within its bounds (a library of interface 1 is refused it)
    const float position = (float)atof(Arg(argc, argv, "--position", "0").c_str());
    if (position != 0.0f && said.iface < 2u) return Die("--position=%g: the library speaks interface %u, which has no position", position, said.iface);
    // (2026-10-18, interface 3) the forms need the store apart
    if ((two || !bare.empty() || !open.empty()) && said.iface < 3u) return Die("--form=%s --bare=%s --open=%s: the library speaks interface %u, which has no NO_STORE and no STORE_ONLY", form.c_str(), bare.c_str(), open.c_str(), said.iface);
    Say("fgtime: the run %s, %d frames, guides %ux%u, %s; the library %s, %llu bytes, sha256 %s, \"%s\", interface %u; position %g%s", run.c_str(), N, fDepth->w, fDepth->h, hud ? "with the HUD-less image" : "no HUD-less image",
        libPath.c_str(), (unsigned long long)libBytes.size(), Sha256(libBytes.data(), libBytes.size()).c_str(), said.build, said.iface, position, position == 0.0f ? " (the middle)" : "");
    Say("fgtime: the form %s%s; bare pairs \"%s\"%s; open pairs \"%s\"; reset at \"%s\"; stamps %s", form.c_str(), two ? (" at t " + std::to_string(t1) + " (NO_STORE" + (twoControl ? " NOT ASKED: THE CONTROL" : "") + ") and " + std::to_string(t2)).c_str() : "",
        bare.c_str(), bareControl ? " (THE CONTROL: no STORE_ONLY call)" : "", open.c_str(), resetAt.c_str(), stamps ? "on" : "off");

    // (2026-10-18, round 3, G3: --gbv=1, THE DEBUG LAYER WITH ITS GPU-BASED VALIDATION, before the first device of the process: the review's note on
    // the count-2 path, two interpolates in one list relying on AMD's backend's barrier between them. --gbv=2 also makes one compute pipeline of
    // junk bytes on the device, THE INSTRUMENT'S OWN CONTROL: the count at the end must hold at least that one message)
    const int gbv = atoi(Arg(argc, argv, "--gbv", "0").c_str());
    if (gbv)
    {
        ID3D12Debug1* d1 = nullptr;
        if (FAILED(D3D12GetDebugInterface(__uuidof(ID3D12Debug1), (void**)&d1)) || !d1) return Die("--gbv: the debug layer is not available here");
        d1->EnableDebugLayer(); d1->SetEnableGPUBasedValidation(TRUE); d1->Release();
        Say("fgtime: the debug layer is on with its GPU-based validation, asked before the first device (--gbv=%d)", gbv);
    }
    Gpu g;
    const uint64_t largest = ((uint64_t)fPres->w * fPres->bpp + 255) / 256 * 256 * fPres->h + 4096;
    if (!GpuMake(&g, largest)) return Die("no Direct3D 12 device, queue or buffers");
    if (gbv >= 2)
    {
        const BYTE junk[16] = { 'n', 'o', 't', ' ', 'a', ' ', 's', 'h', 'a', 'd', 'e', 'r' };
        D3D12_COMPUTE_PIPELINE_STATE_DESC cp = {}; cp.CS.pShaderBytecode = junk; cp.CS.BytecodeLength = sizeof(junk);
        ID3D12PipelineState* ps = nullptr;
        const HRESULT hr = g.dev->CreateComputePipelineState(&cp, __uuidof(ID3D12PipelineState), (void**)&ps);
        if (ps) ps->Release();
        Say("fgtime: the debug layer's control: a compute pipeline of %u junk bytes answered 0x%08X", (unsigned)sizeof(junk), (unsigned)hr);
    }
    UINT64 freq = 0; g.q->GetTimestampFrequency(&freq);
    if (!freq) return Die("the queue has no timestamp frequency");
    ID3D12Resource* tGen = MakeTex(&g, fGen->w, fGen->h, (DXGI_FORMAT)fGen->fmt, true);
    ID3D12Resource* tGen2 = two ? MakeTex(&g, fGen->w, fGen->h, (DXGI_FORMAT)fGen->fmt, true) : nullptr;
    if (two && !tGen2) return Die("the second generated image could not be made");
    std::vector<ID3D12Resource*> tPres(N), tDepth(N), tMv(N), tHud(N, nullptr);
    std::vector<uint8_t> buf;
    for (int i = 0; i < N; i++)
    {
        const FileEnt* e;
        tPres[i] = MakeTex(&g, fPres->w, fPres->h, (DXGI_FORMAT)fPres->fmt, false); tDepth[i] = MakeTex(&g, fDepth->w, fDepth->h, (DXGI_FORMAT)fDepth->fmt, false); tMv[i] = MakeTex(&g, fMv->w, fMv->h, (DXGI_FORMAT)fMv->fmt, false);
        if (!tGen || !tPres[i] || !tDepth[i] || !tMv[i]) return Die("a texture could not be made");
        if (!(e = FindFile(ix, i, "present")) || !ReadAll(run + "\\" + e->name, &buf) || !Upload(&g, tPres[i], buf.data(), buf.size())) return Die("frame %d: its presented picture could not be read or uploaded", i);
        if (!(e = FindFile(ix, i, "depth")) || !ReadAll(run + "\\" + e->name, &buf) || !Upload(&g, tDepth[i], buf.data(), buf.size())) return Die("frame %d: its depth could not be read or uploaded", i);
        if (!(e = FindFile(ix, i, "mv")) || !ReadAll(run + "\\" + e->name, &buf) || !Upload(&g, tMv[i], buf.data(), buf.size())) return Die("frame %d: its vectors could not be read or uploaded", i);
        if (hud && ix.frames[i].kv.at("hud") == "1")
        {
            tHud[i] = MakeTex(&g, fPres->w, fPres->h, (DXGI_FORMAT)fPres->fmt, false);
            if (!tHud[i] || !(e = FindFile(ix, i, "hud")) || !ReadAll(run + "\\" + e->name, &buf) || !Upload(&g, tHud[i], buf.data(), buf.size())) return Die("frame %d: its HUD-less picture could not be read or uploaded", i);
        }
    }

    DlssivFgCreateDesc cd = {};
    cd.size = sizeof(cd); cd.flags = (createFlags & (DLSSIV_FG_CREATE_DEPTH_INVERTED | DLSSIV_FG_CREATE_DEPTH_INFINITE | DLSSIV_FG_CREATE_HDR)) | (stamps ? DLSSIV_FG_CREATE_STAMPS : 0u);
    cd.device = g.dev; cd.displayWidth = fPres->w; cd.displayHeight = fPres->h; cd.maxRenderWidth = fDepth->w; cd.maxRenderHeight = fDepth->h; cd.backBufferFormat = fPres->fmt; cd.stampSlots = 1;
    DlssivFgContext* ctx = nullptr; DlssivFgCreated made = {}; made.size = sizeof(made);
    const uint32_t crc = fnCreate(&cd, &ctx, &made);
    if (crc != DLSSIV_FG_OK || !ctx || made.stamps != (stamps ? 1u : 0u)) return Die("the library's create answered 0x%08X (stamps %u)", crc, made.stamps);

    FILE* fh = nullptr;
    if (!outDir.empty() && dumpFrom >= 0) { fopen_s(&fh, (outDir + "\\hashes.txt").c_str(), "w"); if (!fh) return Die("%s\\hashes.txt could not be written", outDir.c_str()); }
    std::vector<Key> keys; std::vector<std::vector<double>> spans; std::vector<double> whole, wholeInterp;
    std::vector<std::string> callNames; std::vector<std::vector<double>> callSpans;     // (2026-10-18) a call by its place in the list
    std::vector<uint64_t> ticks(DLSSIV_FG_STAMPS_A_SLOT); std::vector<DlssivFgMark> marks(DLSSIV_FG_STAMPS_A_SLOT);
    for (int loop = 0; loop < loops; loop++)
        for (int i = 0; i < N; i++)
        {
            const FrameRec& f = ix.frames[i];
            // (the library resets by itself where a pair's id does not follow the last one's: with --lap-reset=0 the ids run on)
            const uint64_t id = lapReset ? (uint64_t)loop * 100000ull + _strtoui64(f.kv.at("id").c_str(), nullptr, 10)
                                         : _strtoui64(ix.frames[0].kv.at("id").c_str(), nullptr, 10) + (uint64_t)loop * (uint64_t)N + (uint64_t)i;
            const bool lapBegins = i == 0 && (loop == 0 || lapReset);      // (a lap after the first begins with no reset where --lap-reset=0 says so)
            const bool reset = (f.kv.at("disp_reset") == "1" && (i != 0 || lapBegins)) || lapBegins || InList(resetAt, i);
            const bool isBare = !bare.empty() && InList(bare, i), isOpen = !open.empty() && InList(open, i);
            DlssivFgStampsDesc sd = {}; sd.size = sizeof(sd); sd.op = DLSSIV_FG_STAMPS_BEGIN; sd.slot = 0;
            if (!Begin(&g) || (stamps && fnStamps(ctx, &sd) != DLSSIV_FG_OK)) return Die("a list or its stamps would not begin");
            DlssivFgPrepareDesc p = {};
            p.size = sizeof(p); p.flags = ((f.kv.at("prep_reset") == "1" && (i != 0 || lapBegins)) || lapBegins || InList(resetAt, i)) ? DLSSIV_FG_RESET : 0u; p.commandList = g.list; p.depth = tDepth[i]; p.motionVectors = tMv[i]; p.frameId = id;
            {
                const std::vector<std::string> rs = Split(f.kv.at("render"), 'x'), jt = Split(f.kv.at("jitter"), ','), ms = Split(f.kv.at("mvscale"), ','), cam = Split(f.kv.at("cam"), ',');
                p.renderWidth = (uint32_t)atoi(rs[0].c_str()); p.renderHeight = (uint32_t)atoi(rs[1].c_str());
                p.jitterX = FloatOf(jt[0]); p.jitterY = FloatOf(jt[1]); p.motionVectorScaleX = FloatOf(ms[0]); p.motionVectorScaleY = FloatOf(ms[1]);
                p.frameTimeMs = FloatOf(f.kv.at("frame_ms")); p.cameraNear = FloatOf(f.kv.at("near")); p.cameraFar = FloatOf(f.kv.at("far")); p.cameraFovY = FloatOf(f.kv.at("fov")); p.viewSpaceToMeters = FloatOf(f.kv.at("v2m"));
                for (int k = 0; k < 3; k++) { p.cameraPosition[k] = FloatOf(cam[k]); p.cameraUp[k] = FloatOf(cam[3 + k]); p.cameraRight[k] = FloatOf(cam[6 + k]); p.cameraForward[k] = FloatOf(cam[9 + k]); }
            }
            DlssivFgFrameDesc d = {};
            d.size = position != 0.0f ? (uint32_t)sizeof(d) : (uint32_t)DLSSIV_FG_FRAME_SIZE_1; d.position = position;
            d.flags = reset ? DLSSIV_FG_RESET : 0u; d.commandList = g.list; d.present = tPres[i]; d.hudless = tHud[i]; d.output = tGen; d.frameId = id;
            {
                const std::vector<std::string> lum = Split(f.kv.at("lum"), ','), rc = Split(f.kv.at("rect"), ',');
                d.rectLeft = atoi(rc[0].c_str()); d.rectTop = atoi(rc[1].c_str()); d.rectWidth = atoi(rc[2].c_str()); d.rectHeight = atoi(rc[3].c_str());
                d.transfer = (uint32_t)atoi(f.kv.at("transfer").c_str()); d.minLuminance = FloatOf(lum[0]); d.maxLuminance = FloatOf(lum[1]);
            }
            uint32_t rc = fnPrepare(ctx, &p);
            if (rc == DLSSIV_FG_OK) rc = fnFlow(ctx, &d);
            if (InList(resetInterpAt, i)) d.flags |= DLSSIV_FG_RESET;          // (after optical flow: the interpolates alone)
            // (2026-10-18) the pair's form: none (bare), two, one left open, or one
            const char* tagA = "gen-replay"; bool made2 = false, madeA = true;
            if (isBare)
            {
                madeA = false;
                if (!bareControl && rc == DLSSIV_FG_OK)
                {
                    DlssivFgFrameDesc s = d; s.size = (uint32_t)sizeof(s); s.position = 0.0f; s.flags |= DLSSIV_FG_STORE_ONLY; s.output = nullptr;
                    rc = fnInterp(ctx, &s);
                }
            }
            else if (two)
            {
                DlssivFgFrameDesc a = d; a.size = (uint32_t)sizeof(a); a.position = t1; if (!twoControl) a.flags |= DLSSIV_FG_NO_STORE;
                DlssivFgFrameDesc b = d; b.size = (uint32_t)sizeof(b); b.position = t2; b.output = tGen2;
                if (rc == DLSSIV_FG_OK) rc = fnInterp(ctx, &a);
                if (rc == DLSSIV_FG_OK) rc = fnInterp(ctx, &b);
                tagA = "gen-a"; made2 = true;
            }
            else if (isOpen)
            {
                DlssivFgFrameDesc a = d; a.size = (uint32_t)sizeof(a); a.flags |= DLSSIV_FG_NO_STORE;
                if (rc == DLSSIV_FG_OK) rc = fnInterp(ctx, &a);
                // --close=1: the open pair is closed by a call that only keeps it (one generated frame of a pair made in two calls)
                DlssivFgFrameDesc s = d; s.size = (uint32_t)sizeof(s); s.position = 0.0f; s.flags |= DLSSIV_FG_STORE_ONLY; s.output = nullptr;
                if (rc == DLSSIV_FG_OK && closeOpen) rc = fnInterp(ctx, &s);
            }
            else if (rc == DLSSIV_FG_OK) rc = fnInterp(ctx, &d);
            sd.op = DLSSIV_FG_STAMPS_RESOLVE; sd.commandList = g.list;
            if (rc != DLSSIV_FG_OK || (stamps && fnStamps(ctx, &sd) != DLSSIV_FG_OK) || !End(&g)) return Die("frame %d: a call answered 0x%08X or the list did not run", i, rc);
            sd.op = DLSSIV_FG_STAMPS_READ; sd.capacity = DLSSIV_FG_STAMPS_A_SLOT; sd.ticks = ticks.data(); sd.marks = marks.data(); sd.count = 0; sd.lost = 0;
            if (stamps && (fnStamps(ctx, &sd) != DLSSIV_FG_OK || sd.lost || sd.count < 3)) return Die("frame %d: the stamps could not be read (%u read, %u lost)", i, sd.count, sd.lost);
            if (!outDir.empty() && loop == loops - 1 && dumpFrom >= 0 && i >= dumpFrom)
            {
                for (int k = 0; k < 2; k++)
                {
                    if ((k == 0 && !madeA) || (k == 1 && !made2)) continue;
                    const char* tag = k == 0 ? tagA : "gen-b";
                    if (!Readback(&g, k == 0 ? tGen : tGen2, &buf)) return Die("frame %d: its generated image could not be read back", i);
                    fprintf(fh, "f%02d %s %s\n", i, tag, Sha256(buf.data(), buf.size()).c_str()); fflush(fh);
                    char name[64]; _snprintf_s(name, sizeof(name), _TRUNCATE, "\\f%02d-%s.raw", i, tag);
                    // (the second lap's dump, as before 2026-10-18, writes its image always; the forms' runs write one with --raw=1)
                    const bool write = raw || (form == "one" && bare.empty() && open.empty() && resetAt.empty());
                    if (write && !WriteAll(outDir + name, buf.data(), buf.size())) return Die("frame %d: its generated image could not be written to %s%s (%lu)", i, outDir.c_str(), name, GetLastError());
                }
            }
            if (!stamps || loop == 0 || i < from) continue;
            std::map<std::string, int> seen; double interp = 0.0;
            std::map<int, int> callSeen; int callAt = -1;
            for (uint32_t k = 0; k < sd.count; k++)
            {
                if (k == 0)
                {   // (the list's first stamp is the first call's head: it begins that call, and has no span of its own)
                    if (marks[0].kind == DLSSIV_FG_MARK_HEAD)
                    {
                        const int c = marks[0].call, n = ++callSeen[c];
                        const std::string cn = std::string(c == DLSSIV_FG_CALL_PREPARE ? "prepare" : c == DLSSIV_FG_CALL_OPTICAL_FLOW ? "optical flow" : "interpolate") + " #" + std::to_string(n);
                        for (size_t j = 0; j < callNames.size(); j++) if (callNames[j] == cn) callAt = (int)j;
                        if (callAt < 0) { callNames.push_back(cn); callSpans.push_back({}); callAt = (int)callNames.size() - 1; }
                        callSpans[callAt].push_back(0.0);
                    }
                    continue;
                }
                marks[k].name[sizeof(marks[k].name) - 1] = 0;
                const std::string nm = std::string(marks[k].kind == DLSSIV_FG_MARK_HEAD ? "(head) " : marks[k].kind == DLSSIV_FG_MARK_END ? "(end) " : "") + marks[k].name;
                const int nth = seen[std::to_string(marks[k].call) + nm]++;
                size_t at = 0;
                for (; at < keys.size(); at++) if (keys[at].name == nm && keys[at].call == marks[k].call && keys[at].nth == nth) break;
                if (at == keys.size()) { Key key; key.name = nm; key.call = marks[k].call; key.nth = nth; keys.push_back(key); spans.push_back({}); }
                const double us = (double)(ticks[k] - ticks[k - 1]) * 1e6 / (double)freq;
                spans[at].push_back(us);
                if (marks[k].call == DLSSIV_FG_CALL_INTERPOLATE) interp += us;
                // (2026-10-18) THE CALL: a head begins one; its spans, the head's own among them, are summed under its place in the list
                if (marks[k].kind == DLSSIV_FG_MARK_HEAD)
                {
                    const int c = marks[k].call, n = ++callSeen[c];
                    const std::string cn = std::string(c == DLSSIV_FG_CALL_PREPARE ? "prepare" : c == DLSSIV_FG_CALL_OPTICAL_FLOW ? "optical flow" : "interpolate") + " #" + std::to_string(n);
                    callAt = -1;
                    for (size_t j = 0; j < callNames.size(); j++) if (callNames[j] == cn) callAt = (int)j;
                    if (callAt < 0) { callNames.push_back(cn); callSpans.push_back({}); callAt = (int)callNames.size() - 1; }
                    callSpans[callAt].push_back(0.0);
                }
                if (callAt >= 0) callSpans[callAt].back() += us;
            }
            whole.push_back((double)(ticks[sd.count - 1] - ticks[0]) * 1e6 / (double)freq); wholeInterp.push_back(interp);
        }
    fnDestroy(ctx);
    if (fh) fclose(fh);
    if (gbv)
    {   // (round 3, G3) THE DEBUG LAYER'S COUNT over the whole run, by severity, the first twenty in full
        ID3D12InfoQueue* iq = nullptr;
        if (FAILED(g.dev->QueryInterface(__uuidof(ID3D12InfoQueue), (void**)&iq)) || !iq) return Die("--gbv: the device has no message queue: the layer is NOT on it");
        const char* const kSev[5] = { "corruption", "error", "warning", "info", "message" };
        const UINT64 n = iq->GetNumStoredMessages(); UINT64 sev[5] = {};
        for (UINT64 i = 0; i < n; i++)
        {
            SIZE_T len = 0; iq->GetMessage(i, nullptr, &len);
            std::vector<uint8_t> m(len ? len : 1); D3D12_MESSAGE* msg = (D3D12_MESSAGE*)m.data();
            if (!len || FAILED(iq->GetMessage(i, msg, &len))) continue;
            const int sv = msg->Severity >= 0 && msg->Severity <= 4 ? (int)msg->Severity : 4; sev[sv]++;
            if (i < 20) Say("fgtime: d3d12 debug #%llu %s id %d: %.900s", (unsigned long long)i, kSev[sv], (int)msg->ID, msg->pDescription ? msg->pDescription : "");
        }
        Say("fgtime: d3d12 debug with GPU-based validation: %llu messages (%llu corruption, %llu error, %llu warning, %llu info, %llu message)", (unsigned long long)n,
            (unsigned long long)sev[0], (unsigned long long)sev[1], (unsigned long long)sev[2], (unsigned long long)sev[3], (unsigned long long)sev[4]);
        iq->Release();
    }

    if (stamps)
    {
        FILE* fc = nullptr;
        if (!csv.empty()) fopen_s(&fc, csv.c_str(), "w");
        Say("fgtime: %d loops of the run, the steady frames %d to %d of every loop but the first: %d lists. A span's LOWER QUARTILE (and its median), microseconds:", loops, from, N - 1, (int)whole.size());
        double sumQ = 0.0;
        for (size_t k = 0; k < keys.size(); k++)
        {
            const double q = Quartile(spans[k], 0.25), m = Quartile(spans[k], 0.5);
            sumQ += q;
            if (q >= 0.5) Say("  %-9s %-44s %s %7.1f (%7.1f)", keys[k].call == 1 ? "prepare" : keys[k].call == 2 ? "flow" : "interp", keys[k].name.c_str(), keys[k].nth ? ("#" + std::to_string(keys[k].nth + 1)).c_str() : "  ", q, m);
            if (fc) fprintf(fc, "%d,%s,%d,%.2f,%.2f\n", keys[k].call, keys[k].name.c_str(), keys[k].nth, q, m);
        }
        Say("  THE LIST, first stamp to last: %7.1f (%7.1f); the sum of the spans' lower quartiles %7.1f; the interpolate call's spans together %7.1f (%7.1f)", Quartile(whole, 0.25), Quartile(whole, 0.5), sumQ, Quartile(wholeInterp, 0.25), Quartile(wholeInterp, 0.5));
        for (size_t j = 0; j < callNames.size(); j++)
            Say("  THE CALL %-16s its spans together %7.1f (%7.1f), %d lists", callNames[j].c_str(), Quartile(callSpans[j], 0.25), Quartile(callSpans[j], 0.5), (int)callSpans[j].size());
        if (fc) { fprintf(fc, "0,LIST,0,%.2f,%.2f\n", Quartile(whole, 0.25), Quartile(whole, 0.5)); fclose(fc); }
    }
    Say("DONE");
    return 0;
}
