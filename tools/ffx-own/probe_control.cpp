// ffx-own-probe-control.dll: THE PROBE'S CONTROL, A LIBRARY THAT ANSWERS ANOTHER VERSION (2026-10-06, the own build of
// AMD's frame generation, stage 1a).
//
// Ours from the first line to the last: it holds nothing of AMD's but the five names and the types of the public
// ffx-api headers. It exports the five entry points, so it loads and resolves like a real library, and it answers the
// version query with one frame generation provider named "3.1.5" with the id 0xF600000000C01005: the neighbour of the
// answer the probe is asked to find. tools\prove-ffx-own.py runs the probe on it and requires the probe to FAIL. A
// probe that passed this library would pass anything.
//
// It does nothing else: every other call returns an error. It is built into an ignored folder beside the probe and is
// never packaged.
#include <stdint.h>
#include "api/include/ffx_api.h"
#include "framegeneration/include/ffx_framegeneration.h"

ffxReturnCode_t ffxCreateContext(ffxContext*, ffxCreateContextDescHeader*, const ffxAllocationCallbacks*) { return FFX_API_RETURN_NO_PROVIDER; }
ffxReturnCode_t ffxDestroyContext(ffxContext*, const ffxAllocationCallbacks*) { return FFX_API_RETURN_ERROR_PARAMETER; }
ffxReturnCode_t ffxConfigure(ffxContext*, const ffxConfigureDescHeader*) { return FFX_API_RETURN_ERROR_PARAMETER; }
ffxReturnCode_t ffxDispatch(ffxContext*, const ffxDispatchDescHeader*) { return FFX_API_RETURN_ERROR_PARAMETER; }

ffxReturnCode_t ffxQuery(ffxContext* context, ffxQueryDescHeader* header)
{
    if (context || !header || header->type != FFX_API_QUERY_DESC_TYPE_GET_VERSIONS) return FFX_API_RETURN_NO_PROVIDER;
    ffxQueryDescGetVersions* q = (ffxQueryDescGetVersions*)header;
    if (!q->outputCount) return FFX_API_RETURN_OK;
    const bool fg = (q->createDescType & FFX_API_EFFECT_MASK) == FFX_API_EFFECT_ID_FRAMEGENERATION;
    if (!fg) { *q->outputCount = 0; return FFX_API_RETURN_OK; }
    if (*q->outputCount && q->versionIds) q->versionIds[0] = 0xF600000000C01005ull;
    if (*q->outputCount && q->versionNames) q->versionNames[0] = "3.1.5";
    *q->outputCount = 1;
    return FFX_API_RETURN_OK;
}
