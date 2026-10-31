// OUR OWN FILE (not AMD's): the list of providers this library offers, and the two lookups AMD's ffx_api.cpp calls.
//
// AMD's tree at tag v2.3.0 declares GetProvider and GetProviderVersions in api/internal/ffx_provider.h and defines them
// in a file it does not publish (its own signed library's list also holds providers whose source is not public). The
// bodies below hand our list to AMD's own inline lookups in that header, and do nothing else.
//
// THE LIST IS ONE PROVIDER: AMD's analytical frame generation, the one whose source is in
// framegeneration/fsr3/internal/ffx_provider_fsr3framegeneration.cpp. Its name and id are AMD's, read from its source
// and not written here. AMD's frame interpolation swapchain is not in this library and its files are not in the
// repository: the mod presents its own frames.
//
// FFX_BACKEND_DX12 is defined as 0 for every file of this library (build-ffx-own.ps1). DEFINED selects the DX12 headers;
// the VALUE 0 leaves out AMD's driver-side "external provider", whose header (amdinternal/...) is not published either.
// With it out, ffxProviderExternal is a char and the slot below is never filled.
#include "api/internal/ffx_provider.h"
#include "framegeneration/fsr3/include/ffx_provider_fsr3framegeneration.h"

static std::span<ffxProvider* const> OwnProviders()
{
    static ffxProvider* const list[] = {
        &ffxProvider_Fsr3FrameGeneration::GetInstance(),
    };
    return list;
}

ffxProvider* GetProvider(ffxStructType_t descType, uint64_t overrideId, void* device, std::optional<ffxProviderExternal>& extProviderSlot)
{
    return GetProvider(descType, overrideId, device, extProviderSlot, OwnProviders());
}

uint64_t GetProviderVersions(ffxStructType_t descType, void* device, uint64_t capacity, uint64_t* versionIds, const char** versionNames,
                             std::optional<ffxProviderExternal>& extProviderSlot)
{
    return GetProviderVersions(descType, device, capacity, versionIds, versionNames, extProviderSlot, OwnProviders());
}
