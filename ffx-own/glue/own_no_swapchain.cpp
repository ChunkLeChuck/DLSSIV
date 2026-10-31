// OUR OWN FILE (not AMD's): the two functions AMD's backend expects from its frame interpolation swapchain, which this
// library does not contain.
//
// backend/dx12/ffx_dx12.cpp stores both in the backend interface (lines 309 to 315 at tag v2.3.0). AMD's provider calls
// them only when a configure comes WITHOUT FFX_FRAMEGENERATION_FLAG_NO_SWAPCHAIN_CONTEXT_NOTIFY
// (ffx_provider_fsr3framegeneration.cpp 241 to 243 and 440 to 454). A caller that presents its own frames sets that
// flag on every configure, as dlssiv-helper.exe does (host\fg_host.h), and neither function is reached. A caller that
// does not gets an error back, and no crash.
#include "backend/dx12/ffx_dx12.h"

FfxErrorCode ffxSetFrameGenerationConfigToSwapchainDX12(FfxFrameGenerationConfig const*)
{
    return FFX_ERROR_INVALID_ARGUMENT;
}

FfxABIVersion ffxGetSwapchainABIDX12(FfxSwapchain)
{
    return FFX_ABI_INVALID;
}
