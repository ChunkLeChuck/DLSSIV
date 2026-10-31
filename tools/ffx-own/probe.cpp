// ffx-own-probe.exe: ASKS ONE ffx-api LIBRARY WHAT IT OFFERS, WITH NO DEVICE (2026-10-06, the own build of AMD's frame
// generation, stage 1a; grown from the trial's probe).
//
// It loads ONE library by its full path, resolves AMD's five entry points with AMD's own ffx_api_loader.h, the way
// dlssiv-helper.exe does, and asks it for its versions the way the helper's FgProbe does (host\fg_host.h): the count
// first, then the names and ids. The device in every query is null. It creates no context, no Direct3D device, no
// window, and it touches no GPU: AMD's version query walks the library's provider list and returns.
//
//     ffx-own-probe.exe <full path of the library>
//     ffx-own-probe.exe <full path of the library> <name> <id in hex>
//
// With a name and an id it also judges: PASS when the library lists exactly ONE frame generation provider and it is
// that one, FAIL otherwise. What dlssiv-helper.exe logs for AMD's own library is "3.1.6" id 0xF600000000C01006
// (external\ffx\VERSION.txt records it), and tools\prove-ffx-own.py holds ffx-own-fg.dll to the same answer.
//
// Exit code: 0 asked and, if judged, as expected; 1 judged and not as expected; 2 usage; 3 the library did not load or
// lacks an entry point.
#define PLATFORM_WINDOWS
#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "api/include/ffx_api.h"
#include "api/include/ffx_api_loader.h"
#include "upscalers/include/ffx_upscale.h"
#include "framegeneration/include/ffx_framegeneration.h"
#include "framegeneration/include/dx12/ffx_api_framegeneration_dx12.h"

struct Listed { uint64_t n; uint64_t ids[8]; const char* names[8]; ffxReturnCode_t rc; };

static Listed versions(const ffxFunctions& fn, uint64_t type, const char* what)
{
    Listed l = {};
    ffxQueryDescGetVersions q = {};
    q.header.type = FFX_API_QUERY_DESC_TYPE_GET_VERSIONS;
    q.createDescType = type;
    q.device = nullptr;                                   // never a device: this is the whole point
    q.outputCount = &l.n;
    l.rc = fn.Query(nullptr, &q.header);
    if (l.rc == FFX_API_RETURN_OK && l.n)
    {
        if (l.n > 8) l.n = 8;
        q.versionIds = l.ids; q.versionNames = l.names;
        l.rc = fn.Query(nullptr, &q.header);
    }
    printf("%s: query returned %u, %llu provider(s)\n", what, (unsigned)l.rc, (unsigned long long)l.n);
    for (uint64_t i = 0; i < l.n && i < 8; i++) printf("  \"%s\" id 0x%llX\n", l.names[i] ? l.names[i] : "?", (unsigned long long)l.ids[i]);
    return l;
}

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 4) { printf("usage: ffx-own-probe <full path of the library> [<name> <id in hex>]\n"); return 2; }
    const char* path = argv[1];
    if (!(strlen(path) > 3 && path[1] == ':' && (path[2] == '\\' || path[2] == '/'))) { printf("FAIL: not a full path: %s\n", path); return 2; }
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);   // never a dialog
    HMODULE m = LoadLibraryA(path);
    if (!m) { printf("FAIL: the library did not load (error %lu): %s\n", GetLastError(), path); return 3; }
    printf("library: %s\n", path);
    ffxFunctions fn = {};
    ffxLoadFunctions(&fn, m);
    printf("entry points: create %d destroy %d configure %d query %d dispatch %d\n", !!fn.CreateContext, !!fn.DestroyContext, !!fn.Configure, !!fn.Query, !!fn.Dispatch);
    if (!fn.CreateContext || !fn.DestroyContext || !fn.Configure || !fn.Query || !fn.Dispatch) { printf("FAIL: an entry point is missing\n"); return 3; }
    Listed fg = versions(fn, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATION, "frame generation");
    versions(fn, FFX_API_CREATE_CONTEXT_DESC_TYPE_FRAMEGENERATIONSWAPCHAIN_WRAP_DX12, "frame generation swapchain");
    versions(fn, FFX_API_CREATE_CONTEXT_DESC_TYPE_UPSCALE, "upscale");
    int rc = 0;
    if (argc == 4)
    {
        const uint64_t want = _strtoui64(argv[3], nullptr, 16);
        const bool ok = fg.rc == FFX_API_RETURN_OK && fg.n == 1 && fg.names[0] && !strcmp(fg.names[0], argv[2]) && fg.ids[0] == want;
        if (ok) printf("PASS: one frame generation provider, \"%s\" id 0x%llX\n", argv[2], (unsigned long long)want);
        else printf("FAIL: wanted one frame generation provider, \"%s\" id 0x%llX; the library lists %llu, the first \"%s\" id 0x%llX\n", argv[2], (unsigned long long)want,
                    (unsigned long long)fg.n, fg.n && fg.names[0] ? fg.names[0] : "none", (unsigned long long)(fg.n ? fg.ids[0] : 0));
        rc = ok ? 0 : 1;
    }
    FreeLibrary(m);
    return rc;
}
