// THE HUD-LESS PICTURE'S PICTURE PROOF (2026-10-10, frame generation phase 3, the build map's stage 15): the encodes of
// source/fg/shaders drawn on the GPU through DXVK, with no game, over the real encode's own output, and read back pixel for
// pixel. tools/test-hdr-sharpen/g10.cpp is the harness: its frame, its states, its draw and its compare are used as they stand
// (the file is included whole, its main renamed).
// (THE FORM OF BEFORE, the HUD-less pass drawn over the real encode into the back buffer, and its proofs a, b and f, its
// faint-interface answer and its cost rows went with the pass and its five shaders in round 8's deep strip
// (docs\INSTRUMENTS-FG.md): this file at the tag fg-instruments has them.)
//
// THE SECOND-TARGET FORM (2026-10-11; hdr_encode_ps.hlsl, HDR_FGMRT; source/fg/fg_hud.h, "WHERE IT IS DRAWN"): the four encodes
// of source/fg/shaders that write the presented picture and the HUD-less picture in ONE draw into two targets, in the
// same six states at the same two UI levels:
//   c  ITS PRESENTED PICTURE (target 0) IS THE SHIPPING ENCODE'S on every pixel (measured: bit for bit in every state);
//   d  OUTSIDE THE INTERFACE ITS HUD-LESS PICTURE (target 1) IS ITS OWN PRESENTED PICTURE BIT FOR BIT, 0 pixels differ;
//   e  INSIDE THE INTERFACE IT IS THE REAL ENCODE OF THE FRAME BEFORE THE INTERFACE, on 99 percent or more of its pixels;
//   g  WITH THE DEVICE'S OWN BACK BUFFER AS TARGET 0, as the game binds it, the one draw writes both targets;
//   h  THE WORKER'S COPY OUT OF THE SECOND TARGET BY THE PRODUCT'S OWN VULKAN LINES on DXVK's queue (RawProof has the order):
//      the raw image holds the frame's HUD-less picture bit for bit, and the next frame's encode draws into the target
//      again. Run a second time with --layouts, where DXVK keeps its images in layouts of their kind.
//   p  THE PHONE (2026-10-12): a blended 3D overlay drawn ahead of the interface, with the copy taken ahead of it (the armed
//      product since that day) and where it was (the control): PhoneProof says what is held.
// Each with its controls (the interface's own pixels, another paper white, no second target bound, the next frame's picture).
// --time, SINCE THEN, ALSO TIMES WHOLE FRAMES: the game's side of one pair in the order the game makes it (the frame, the
// encode's copy, the encode, the copies of the hand, and each form's own work), 300 frames back to back, for the two-image
// set, the second target, and a frame with no interface. A form's cost is its frame less the two-image frame's.
// (One thing drawn 300 times over the same pixels, as the lines before it do, is not what a frame costs: the note there.)
//
//     tools\test-fg-hudless\build.ps1         builds hudless.exe (x86, as the game) in a folder of its own under %TEMP%, and
//                                             the same file as vklines_hudless.exe, a name the one-GPU-user rule lists
//     hudless.exe --d3d9 <path> [--time] [--layouts]   <path>: DXVK's d3d9.dll (native Direct3D 9 refuses this draw, as it does G10's)
// Exit 0 when every proof passed. NO WINDOW IS SHOWN: the device's window is 64 by 64, made and never shown. It changes
// nothing on the machine.
#define main G10Main
#include "../test-hdr-sharpen/g10.cpp"
#undef main
#include <string>
#include "../../source/fg/shaders/hdr_fgmrt_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_cga_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_g10_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_cga_g10_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_ov_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_ov_cga_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_ov_g10_ps.h"
#include "../../source/fg/shaders/hdr_fgmrt_ov_cga_g10_ps.h"
#include "../../source/shaders/hdr_cgablit_ps.h"      // (SDR, proofs u and v: the mod's reproduction of FusionFix's console gamma pass stands for FusionFix's shader)
#include "../../source/fg/fg_hud_sdr_d3d.h"           // (SDR, round 2: THE MOD'S OWN second draw of the curve, called by proofs u, v and w)

// THE SECOND-TARGET FORM (2026-10-11; hdr_encode_ps.hlsl, HDR_FGMRT): the real encode drawn ONCE with two targets bound, the
// presented picture into the first and the HUD-less picture into the second. Target 1 is bound round g10.cpp's own Draw,
// which binds target 0 and everything else.
static void DrawMrt(Dev& v, IDirect3DPixelShader9* ps, const float (&c)[9][4], IDirect3DTexture9* rt0, IDirect3DTexture9* rt1, IDirect3DTexture9* const (&tx)[12])
{
    IDirect3DSurface9* s1 = nullptr; CK(rt1->GetSurfaceLevel(0, &s1));
    CK(v.d->SetRenderTarget(1, s1)); s1->Release();
    Draw(v, ps, c, rt0, tx);
    CK(v.d->SetRenderTarget(1, nullptr));
}
// THE PRODUCT'S OWN VULKAN LINES (proof h): the transport's loader and source\fg\fg_vklines.h, as the armed product includes
// them. (build.ps1 gives the three folders they are read from.)
#pragma warning(push, 3)
#pragma warning(disable: 4505)
#include "feed_vk.h"
#include "../../source/dxvk/dxvk_interop_min.h"
#include "fg_vklines.h"
#pragma warning(pop)
#pragma warning(disable: 4505)   // (the lines this tool does not call)

struct Out2 { Out c0, c1; };
static Out2 RenderMrt(Dev& v, IDirect3DPixelShader9* mrt, const float (&c)[9][4], IDirect3DTexture9* const (&tx)[12], IDirect3DTexture9* rt32b, IDirect3DTexture9* rt16b)
{
    Out2 o;
    DrawMrt(v, mrt, c, v.rt32, rt32b, tx); o.c0.f32 = Read<float>(v, v.rt32, v.sys32); o.c1.f32 = Read<float>(v, rt32b, v.sys32);
    DrawMrt(v, mrt, c, v.rt16, rt16b, tx); o.c0.f16 = Read<uint16_t>(v, v.rt16, v.sys16); o.c1.f16 = Read<uint16_t>(v, rt16b, v.sys16);
    return o;
}

// The real encode into each target; both targets read back.
static Out Render2(Dev& v, IDirect3DPixelShader9* real, const float (&c)[9][4], IDirect3DTexture9* const (&tx)[12])
{
    Out o;
    Draw(v, real, c, v.rt32, tx); o.f32 = Read<float>(v, v.rt32, v.sys32);
    Draw(v, real, c, v.rt16, tx); o.f16 = Read<uint16_t>(v, v.rt16, v.sys16);
    return o;
}
static bool PixelDiffers(const Out& A, const Out& B, size_t i)
{
    for (int c = 0; c < 3; ++c)
    {
        const float a = A.f32[4 * i + c], b = B.f32[4 * i + c];
        if (std::isnan(a) || std::isnan(b)) { if (std::isnan(a) != std::isnan(b)) return true; }
        else if (Ulps(a, b)) return true;
        if (A.f16[4 * i + c] != B.f16[4 * i + c]) { const uint16_t ha = A.f16[4 * i + c], hb = B.f16[4 * i + c];
            const bool na = (ha & 0x7C00u) == 0x7C00u && (ha & 0x3FFu), nb = (hb & 0x7C00u) == 0x7C00u && (hb & 0x3FFu); if (!(na && nb)) return true; }
    }
    return false;
}
// h  THE WORKER'S COPY OUT OF THE SECOND TARGET, BY THE PRODUCT'S OWN LINES, ON DXVK'S OWN QUEUE (2026-10-11). What the armed
//    product's hand does for a frame whose HUD-less picture the encode wrote into its second target (fg_machine.h,
//    ArmedSpan; fg_arm.h, FgPA::RecCopyHudToImg), in its order: the encode draws with two targets; THE FLUSH
//    (ID3D9VkInteropDevice::FlushRenderingCommands); the target's Vulkan image and the layout DXVK holds it in, asked of
//    DXVK (FgvBbQuery); the submission lock; ONE BUFFER with FgvRecCopyOut out of that image into a raw image of this
//    program's own (standing for the helper's HUDLESS), submitted on DXVK's queue; the lock released. THEN THE NEXT FRAME'S
//    ENCODE DRAWS INTO THE SAME TARGET, so DXVK meets the image where the copy left it. The raw image is then brought
//    back by the product's copy in (FgvRecCopyIn) into a Direct3D 9 target and read, and the second target is read too.
//    PROVED: the raw image holds THE FIRST FRAME'S HUD-less picture bit for bit (every pixel, FP16), the second target the
//    second frame's. CONTROL: the same compare against the second frame's picture must fail.
//    Run once as DXVK lays its images out on this driver and once with --layouts (dxvk.enableUnifiedImageLayouts = False:
//    what a driver without that extension gets): the layout DXVK reports for the target is printed.
//    (2026-10-12, the phone's cover: tx is the frame's textures, and what the HUD-less picture's ALPHA held is counted too. The compare
//    was always of all four channels, eight bytes a pixel; with the overlay's encode the alpha is 0 on the cover.)
static void RawProof(Dev& v, IDirect3DPixelShader9* mrtPs, IDirect3DTexture9* t2, const char* mode, IDirect3DTexture9* const (&tx)[12], const char* tag = "h ")
{
    ID3D9VkInteropDevice* idev = nullptr;
    if (FAILED(v.d->QueryInterface(__uuidof(ID3D9VkInteropDevice), (void**)&idev)) || !idev) { Proof(false, "h  the device answers no ID3D9VkInteropDevice: it is not DXVK's"); return; }
    VkInstance inst = nullptr; VkPhysicalDevice phys = nullptr; VkDevice dev = nullptr; idev->GetVulkanHandles(&inst, &phys, &dev);
    VkQueue q = nullptr; uint32_t qi = 0, fam = 0; idev->GetSubmissionQueue(&q, &qi, &fam);
    FeedVk vk; const char* miss = nullptr;
    if (!dev || !q || !FeedVkLoad(&vk, dev, &miss)) { Proof(false, "h  the transport's loader did not take its entry points on DXVK's device (%s)", miss ? miss : "no device or queue"); idev->Release(); return; }
#define GDP(n) const PFN_##n n##_ = (PFN_##n)vk.GetDeviceProcAddr(dev, #n)
    GDP(vkCreateCommandPool); GDP(vkDestroyCommandPool); GDP(vkAllocateCommandBuffers); GDP(vkBeginCommandBuffer); GDP(vkEndCommandBuffer); GDP(vkQueueSubmit);
    GDP(vkCreateFence); GDP(vkDestroyFence); GDP(vkWaitForFences);
#undef GDP
    const HMODULE vl = GetModuleHandleA("vulkan-1.dll");
    const PFN_vkGetInstanceProcAddr gipa = vl ? (PFN_vkGetInstanceProcAddr)GetProcAddress(vl, "vkGetInstanceProcAddr") : nullptr;
    const PFN_vkGetPhysicalDeviceMemoryProperties gmp = gipa ? (PFN_vkGetPhysicalDeviceMemoryProperties)gipa(inst, "vkGetPhysicalDeviceMemoryProperties") : nullptr;
    if (!vkCreateCommandPool_ || !vkAllocateCommandBuffers_ || !vkBeginCommandBuffer_ || !vkEndCommandBuffer_ || !vkQueueSubmit_ || !vkCreateFence_ || !vkWaitForFences_ || !gmp)
    { Proof(false, "h  a Vulkan entry point this proof needs is missing"); idev->Release(); return; }
    VkPhysicalDeviceMemoryProperties mp = {}; gmp(phys, &mp);
    FgvImage X = {};
    VkCommandPool pool = VK_NULL_HANDLE; VkCommandBuffer cb[2] = {}; VkFence fence[2] = {};
    VkCommandPoolCreateInfo pci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; pci.queueFamilyIndex = fam;
    VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    bool ok = FgvImageMake(&vk, mp, (UINT)W, (UINT)H, VK_FORMAT_R16G16B16A16_SFLOAT, &X) && vkCreateCommandPool_(dev, &pci, nullptr, &pool) == VK_SUCCESS;
    VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 2;
    ok = ok && vkAllocateCommandBuffers_(dev, &cai, cb) == VK_SUCCESS && vkCreateFence_(dev, &fci, nullptr, &fence[0]) == VK_SUCCESS && vkCreateFence_(dev, &fci, nullptr, &fence[1]) == VK_SUCCESS;
    if (!ok) { Proof(false, "h  this proof's own Vulkan objects could not be made"); idev->Release(); return; }
    // the two frames, by Direct3D 9 alone: A at paper white 203, B at 100
    State s; Base(s, "raw", false, 1.0f, true); s.c[8][0] = 0.0f;
    float cA[9][4]; memcpy(cA, s.c, sizeof(cA)); cA[1][1] = 1.0f;
    float cB[9][4]; memcpy(cB, cA, sizeof(cB)); cB[0][0] = 100.0f;
    IDirect3DTexture9* R = nullptr; CK(v.d->CreateTexture(W, H, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &R, nullptr));
    IDirect3DSurface9 *t2S = nullptr, *rS = nullptr; CK(t2->GetSurfaceLevel(0, &t2S)); CK(R->GetSurfaceLevel(0, &rS));
    DrawMrt(v, mrtPs, cA, v.rt16, t2, tx);
    const std::vector<uint16_t> refA = Read<uint16_t>(v, t2, v.sys16), presA = Read<uint16_t>(v, v.rt16, v.sys16);
    DrawMrt(v, mrtPs, cB, v.rt16, t2, tx);
    const std::vector<uint16_t> refB = Read<uint16_t>(v, t2, v.sys16);
    CK(v.d->ColorFill(t2S, nullptr, 0)); CK(v.d->ColorFill(rS, nullptr, 0));
    // THE SEQUENCE
    auto begin = [&](VkCommandBuffer c) { VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; return vkBeginCommandBuffer_(c, &bi) == VK_SUCCESS; };
    auto submit = [&](VkCommandBuffer c, VkFence f) { VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.commandBufferCount = 1; si.pCommandBuffers = &c; return vkEndCommandBuffer_(c) == VK_SUCCESS && vkQueueSubmit_(q, 1, &si, f) == VK_SUCCESS; };
    DrawMrt(v, mrtPs, cA, v.rt16, t2, tx);                     // frame A's encode: the presented picture and, into the second target, its HUD-less picture
    idev->FlushRenderingCommands();                            // the hand's span: the flush
    VkImage tImg = VK_NULL_HANDLE; VkImageLayout tLay = VK_IMAGE_LAYOUT_UNDEFINED; UINT tw = 0, th = 0; VkFormat tf = VK_FORMAT_UNDEFINED;
    const bool q1 = FgvBbQuery(t2S, &tImg, &tLay, &tw, &th, &tf);
    idev->LockSubmissionQueue();
    bool s1 = begin(cb[0]);
    FeedVkBarrierN(&vk, cb[0], &X.img, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
    const bool rec1 = q1 && FgvRecCopyOut(&vk, cb[0], tImg, tLay, X.img, false, fam, tw, th);
    s1 = s1 && submit(cb[0], fence[0]);
    idev->ReleaseSubmissionQueue();
    DrawMrt(v, mrtPs, cB, v.rt16, t2, tx);                     // frame B's encode, into the target the copy has just read
    idev->FlushRenderingCommands();
    VkImage rImg = VK_NULL_HANDLE; VkImageLayout rLay = VK_IMAGE_LAYOUT_UNDEFINED; UINT rw = 0, rh = 0; VkFormat rf = VK_FORMAT_UNDEFINED;
    const bool q2 = FgvBbQuery(rS, &rImg, &rLay, &rw, &rh, &rf);
    idev->LockSubmissionQueue();
    bool s2 = begin(cb[1]);
    const bool rec2 = q2 && FgvRecCopyIn(&vk, cb[1], X.img, false, fam, rImg, rLay, rw, rh);
    s2 = s2 && submit(cb[1], fence[1]);
    idev->ReleaseSubmissionQueue();
    const bool retired = vkWaitForFences_(dev, 2, fence, VK_TRUE, 10000000000ull) == VK_SUCCESS;
    const std::vector<uint16_t> gotR = Read<uint16_t>(v, R, v.sys16), gotT = Read<uint16_t>(v, t2, v.sys16);
    size_t nA = 0, nB = 0, ctl = 0, picDiffers = 0, aZero = 0, aZeroGot = 0, aOther = 0; const size_t n = (size_t)W * H;
    for (size_t i = 0; i < n; ++i)
    {
        nA  += memcmp(&gotR[4 * i], &refA[4 * i], 8) ? 1 : 0;
        nB  += memcmp(&gotT[4 * i], &refB[4 * i], 8) ? 1 : 0;
        ctl += memcmp(&gotR[4 * i], &refB[4 * i], 8) ? 1 : 0;
        picDiffers += memcmp(&refA[4 * i], &presA[4 * i], 6) ? 1 : 0;
        if (refA[4 * i + 3] == 0) { aZero++; aZeroGot += gotR[4 * i + 3] == 0 ? 1 : 0; } else if (refA[4 * i + 3] != ToHalf(1.0f)) aOther++;
    }
    Proof(q1 && q2 && rec1 && rec2 && s1 && s2 && retired && tw == (UINT)W && th == (UINT)H && tf == VK_FORMAT_R16G16B16A16_SFLOAT && nA == 0 && nB == 0 && picDiffers * 20 >= n && aZeroGot == aZero && aOther == 0,
          "%s the worker's copy out of the second target by the product's own lines, %s (DXVK reports the target in VkImageLayout %d, format %d, %u by %u): behind the flush the raw image holds the frame's HUD-less picture (%zu of %zu pixels differ),"
          " the next frame's encode then drew into the same target (%zu of %zu pixels differ from its picture); that picture differs from the presented one on %zu pixels; ITS ALPHA: 0 on %zu pixels of the frame's picture and on %zu of them in the raw image, 1 on every other (%zu are neither); queries %d %d, recorded %d %d, submitted %d %d, retired %d",
          tag, mode, (int)tLay, (int)tf, tw, th, nA, n, nB, n, picDiffers, aZero, aZeroGot, aOther, (int)q1, (int)q2, (int)rec1, (int)rec2, (int)s1, (int)s2, (int)retired);
    Proof(ctl * 2 > n, "   control: the same compare against the NEXT frame's picture fails on %zu of %zu pixels", ctl, n);
    FgvImageFree(&vk, &X);
    vkDestroyFence_(dev, fence[0], nullptr); vkDestroyFence_(dev, fence[1], nullptr); vkDestroyCommandPool_(dev, pool, nullptr);
    rS->Release(); t2S->Release(); R->Release(); idev->Release();
}
// p  THE PHONE (2026-10-12; source/fg/fg_hud.h, "THE PHONE" and "TWO COPY POINTS"; docs/reports/fg-3/sitting-2026-10-12c/BRIEF-phone.md and
//    docs/reports/fg-4/brief-2026-10-12/BRIEF-phone-cover.md). The game draws its phone as a blended 3D model into the back buffer
//    ahead of the 2D interface. THE ENCODE'S OWN COPY is taken at the first sprite, BEHIND the phone, as in a release; FRAME
//    GENERATION'S COPY is taken AHEAD of the phone's first draw, into a texture of its own; a frame with both is drawn by the
//    encodes of HDR_FGOV, which read the second on a sampler of their own and the phone's rect in a constant row.
//    THE PICTURES DRAWN HERE, with the product's own encodes (plain): a scene; a phone blended over it in a corner with a soft
//    edge 24 pixels wide (its alpha from 2 percent to 1), inside a viewport larger than itself; then the HUD. PLANTED
//    BESIDE THE PHONE, inside its viewport: four pixels where the frame behind the phone differs from the frame ahead of it
//    by ONE STEP OF THE HALF FLOAT in one channel, at four magnitudes; and OUTSIDE its viewport a patch that differs by a
//    lot (something else drawn between the two copy points, a corona).
//    p  THE COVER IS EXACT: the second output's alpha is 0 on every pixel inside the viewport where the two copies differ at
//       all (FP16 bits OF ANY OF THE FOUR CHANNELS since 2026-10-13, the planted single steps among them, and a block where the
//       phone's colour is the scene's and its draw moved the back buffer's fourth channel alone) and 1 on every other
//       pixel (a second block of the phone's own colour where no channel differs among them: no copy can say it); on every pixel of the cover the
//       HUD-less picture is the shipping encode of THE FRAME BEFORE THE PHONE; off the cover and off the HUD the two pictures
//       are one, bit for bit, the patch outside the viewport with them (scene to both); the HUD is taken out by the
//       rule it has. AMD's own line (ffx_frameinterpolation_common.h, CalculateStaticContentFactor) is printed for the
//       cover: where it reads under 1 the stock rule would show the interpolated scene in part, which the mod's own
//       library no longer asks (it reads the alpha: ffx-own/cuts/fi-cover-in-alpha.cut).
//       CONTROLS: an empty rect: no pixel is cover, and the phone stands in the HUD-less picture; THE ENCODE'S OWN COPY ALONE (the
//       encode every other frame draws with): the HUD-less picture holds the PHONE bit for bit, alpha 1.
//    q  THE REAL PICTURE OF SUCH A FRAME IS A RELEASE'S: target 0 against the shipping encode with the release's copy, every
//       pixel, at the UI level 1 and at 0.15 (a level that shows any coverage). Its alpha is 1 on every pixel.
//       CONTROL, FAULT 1 AS IT STOOD: the shipping encode with the copy AHEAD of the phone as its own (the first form, eca6ac9b)
//       differs from a release's on the phone's pixels (its coverage is above zero there); and with the release's copy
//       the level changes no bit of them (its coverage is zero there).
//    s  THE GHOSTING FIX STANDS UNDER THE COVER (the owner, 2026-10-12, 22:25: "the ghosting fix worked on it, I dont want to mess with
//       that"; 2026-10-13: the cover is asked for beside it). Target 1 of the overlay's encode against target 1 of the first
//       form (eca6ac9b: the encode every frame draws with, the copy ahead of the phone as the encode's own), at both UI
//       levels: OFF THE COVER EVERY PIXEL IS THE FIRST FORM'S, and its alpha is 1; ON THE COVER every pixel where the first form
//       held the frame before the phone holds it still (the same picture there), and the pixels the first form LEFT IN
//       BOTH PICTURES (the mask's dead zone: the see-through) are counted: the cover holds the frame before the phone on
//       those too. CONTROL: against the encode's own copy alone the compare off the cover holds and the one on it fails.
//       THE ONE PLACE THE TWO FORMS PART, SAID AND COUNTED: something else drawn between the two copy points OUTSIDE the overlay's rect (the
//       patch). The first form took it out of the HUD-less picture with the phone; the cover's form leaves it scene to both
//       pictures. The owner's leg of 2026-10-13 counted no frame with a scene draw there in 123 windows with the phone up.
//    r  THE OVERLAY'S COPY ALONE (the game's HUD off: no copy of the encode's): the encode every frame draws with, the
//       frame ahead of the phone bound where the encode's copy would be, the real picture's mask off as the mod has
//       it on such a frame: target 0 is the shipping encode's, target 1 holds the frame ahead of the phone where the
//       mask says so (the first form's picture: its dead zone stands), alpha 1 everywhere.
static void OvRect(Dev& v, int x0, int y0, int x1, int y1)
{
    const float r[4] = { (float)x0, (float)y0, (float)x1, (float)y1 };
    CK(v.d->SetPixelShaderConstantF(9, r, 1));                 // gFgOv (hdr_encode_ps.hlsl, HDR_FGOV): a row beyond the nine Draw uploads
}
// (2026-10-13, the cover reads all four channels) the fourth channel of a managed FP16 texture, over a rect
static void SetAlphaRect(IDirect3DTexture9* t, int x0, int y0, int x1, int y1, float a)
{
    D3DLOCKED_RECT lr; CK(t->LockRect(0, &lr, nullptr, 0));
    for (int y = y0; y < y1; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch); for (int x = x0; x < x1; ++x) row[4 * x + 3] = ToHalf(a); }
    t->UnlockRect(0);
}
static void PhoneProof(Dev& v, IDirect3DPixelShader9* enc, IDirect3DPixelShader9* mrtPs, IDirect3DPixelShader9* mrtOvPs, IDirect3DTexture9* rt32b, IDirect3DTexture9* rt16b, const char* mode)
{
    const int X0 = 640, Y0 = 224, X1 = 896, Y1 = 480; const float RAMP = 24.0f;
    const int VX0 = 576, VY0 = 160, VX1 = 960, VY1 = 544;                               // the phone's viewport: larger than the phone
    // TWO BLOCKS WHERE THE PHONE'S COLOUR IS THE SCENE'S BEHIND IT (2026-10-13), 12 by 12 each, inside the part it covers in full: the three
    // colour channels of the two copies are equal there. In EA the phone's draw moved the back buffer's FOURTH channel
    // (1 ahead of the phone, 0.5 behind it), as a blended draw that writes all four channels does: the cover must say
    // those pixels. In EB no channel differs: no copy can say them, they are scene to both pictures (the limit, said).
    const int EAX = 700, EBX = 720, EY = 300, EN = 12;
    auto inEA = [&](int x, int y) { return x >= EAX && x < EAX + EN && y >= EY && y < EY + EN; };
    auto inEB = [&](int x, int y) { return x >= EBX && x < EBX + EN && y >= EY && y < EY + EN; };
    auto alpha = [&](int x, int y) -> float {
        if (x < X0 || x >= X1 || y < Y0 || y >= Y1) return 0.0f;
        const float d = (float)std::min(std::min(x - X0, X1 - 1 - x), std::min(y - Y0, Y1 - 1 - y)) + 0.5f;
        return std::min(d / RAMP, 1.0f); };
    auto inView = [&](int x, int y) { return x >= VX0 && x < VX1 && y >= VY0 && y < VY1; };
    auto lerp3 = [](const Px& a, const Px& b, float t) { return Px{ a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t }; };
    std::vector<Px> S((size_t)W * H);
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
        S[(size_t)y * W + x] = { 0.30f + 0.25f * (x + 0.5f) / W + 0.02f * Rnd(), 0.35f + 0.20f * (y + 0.5f) / H + 0.02f * Rnd(), 0.45f + 0.02f * Rnd() };
    // the four planted pixels: the scene's value there, at four magnitudes (each exactly a half float)
    const int PX[4] = { 600, 604, 608, 612 }, PY = 180; const float PV[4] = { FromHalf(0x0200), 0.5f, 4.0f, 60.0f };     // (0x0200: a subnormal half, 3.05e-5)
    for (int k = 0; k < 4; ++k) S[(size_t)PY * W + PX[k]] = { PV[k], PV[k], PV[k] };
    std::vector<Px> FP = S;                                    // the scene with the phone on it: the frame at the first sprite
    for (int y = Y0; y < Y1; ++y) for (int x = X0; x < X1; ++x)
    {
        const bool screen = x >= X0 + 64 && x < X1 - 64 && y >= Y0 + 64 && y < Y1 - 64;      // its lit screen, in a dark body
        Px& p = FP[(size_t)y * W + x]; if (!inEA(x, y) && !inEB(x, y)) p = lerp3(p, screen ? Px{ 0.20f, 0.75f, 0.95f } : Px{ 0.04f, 0.04f, 0.05f }, alpha(x, y));
    }
    for (int k = 0; k < 4; ++k) { Px& p = FP[(size_t)PY * W + PX[k]]; const float up = FromHalf((uint16_t)(ToHalf(PV[k]) + 1)); if (k == 0) p.r = up; else if (k == 1) p.g = up; else if (k == 2) p.b = up; else p.r = up; }
    const int CX0 = 96, CY0 = 384, CX1 = 128, CY1 = 416;        // outside the viewport: something else drawn between the two copy points
    for (int y = CY0; y < CY1; ++y) for (int x = CX0; x < CX1; ++x) { Px& p = FP[(size_t)y * W + x]; p = { p.r + 0.30f, p.g + 0.30f, p.b + 0.20f }; }
    std::vector<Px> F = FP;                                    // and the HUD on that: the presented frame
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) if (InHud(x, y)) { Px& p = F[(size_t)y * W + x]; p = lerp3(p, Px{ 0.90f, 0.90f, 0.20f }, 0.7f); }
    IDirect3DTexture9 *tS = MakeHalfTex(v.d, W, H, S), *tFP = MakeHalfTex(v.d, W, H, FP), *tF = MakeHalfTex(v.d, W, H, F);
    SetAlphaRect(tFP, EAX, EY, EAX + EN, EY + EN, 0.5f);       // (the encode's own copy, behind the phone: its fourth channel where the phone drew over its own colour)
    IDirect3DTexture9* txNew[12]; memcpy(txNew, v.tex, sizeof(txNew)); txNew[0] = tF; txNew[11] = tFP; txNew[5] = tS;   // BOTH COPIES: the encode's own at the first sprite, frame generation's AHEAD of the phone
    IDirect3DTexture9* txOld[12]; memcpy(txOld, v.tex, sizeof(txOld)); txOld[0] = tF; txOld[11] = tFP;    // the encode's own copy alone: at the first sprite, behind the phone
    IDirect3DTexture9* txRef[12]; memcpy(txRef, v.tex, sizeof(txRef)); txRef[0] = tS; txRef[11] = tS;     // the frame before the phone, by the shipping encode
    IDirect3DTexture9* txWas[12]; memcpy(txWas, v.tex, sizeof(txWas)); txWas[0] = tF; txWas[11] = tS;     // the first form: the copy ahead of the phone AS THE ENCODE'S OWN
    IDirect3DTexture9* txOff[12]; memcpy(txOff, v.tex, sizeof(txOff)); txOff[0] = tFP; txOff[11] = tS;    // the HUD off: the frame ends with the phone; the overlay's copy where the encode's would be
    IDirect3DTexture9* txOffR[12]; memcpy(txOffR, v.tex, sizeof(txOffR)); txOffR[0] = tFP; txOffR[11] = tFP;
    State s; Base(s, "phone", false, 1.0f, true); s.c[8][0] = 0.0f;
    float c[9][4]; memcpy(c, s.c, sizeof(c)); c[1][1] = 1.0f;
    float cLow[9][4]; memcpy(cLow, c, sizeof(cLow)); cLow[1][1] = 0.15f;      // a UI level that shows any coverage of the real picture's mask
    float cNo[9][4]; memcpy(cNo, c, sizeof(cNo)); cNo[1][1] = 0.0f;           // no copy of the encode's this frame: its mask is off (hdr_encode.h, c[1][1])
    OvRect(v, VX0, VY0, VX1, VY1);
    const Out2 N = RenderMrt(v, mrtOvPs, c, txNew, rt32b, rt16b), Nlow = RenderMrt(v, mrtOvPs, cLow, txNew, rt32b, rt16b);
    OvRect(v, 0, 0, 0, 0);
    const Out2 E = RenderMrt(v, mrtOvPs, c, txNew, rt32b, rt16b);              // the control: an empty rect
    const Out2 O = RenderMrt(v, mrtPs, c, txOld, rt32b, rt16b);
    const Out R = Render2(v, enc, c, txRef);
    // AMD's own line, on the two pictures as the helper is handed them (FP16)
    auto factor = [](const Out& hudless, const Out& present, size_t i) {
        float m = 0.0f;
        for (int k = 0; k < 3; ++k)
        {
            const float a = FromHalf(hudless.f16[4 * i + k]), b = FromHalf(present.f16[4 * i + k]), mx = std::max(a, b);
            m = std::max(m, std::min(std::max((1.0f - (mx != 0.0f ? std::min(a, b) / mx : 0.0f)) / 0.1f, 0.0f), 1.0f));
        }
        return m; };
    auto solid = [&](int x, int y) { return alpha(x, y) >= 1.0f && !inEA(x, y) && !inEB(x, y); };   // (covered in full AND of another colour than the scene)
    // THE COVER, as the textures hold the two copies: FP16 bits of the frame ahead of the phone against the frame behind it, ALL FOUR
    // CHANNELS, inside the viewport (the fourth differs in EA alone)
    std::vector<uint8_t> cover((size_t)W * H, 0), differs((size_t)W * H, 0);
    size_t nCover = 0, nDiffOut = 0, nSolid = 0, nEdge = 0, nOwn = 0;
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
    {
        const size_t i = (size_t)y * W + x;
        const bool d = ToHalf(S[i].r) != ToHalf(FP[i].r) || ToHalf(S[i].g) != ToHalf(FP[i].g) || ToHalf(S[i].b) != ToHalf(FP[i].b) || inEA(x, y);
        differs[i] = d ? 1 : 0;
        if (d && inView(x, y)) { cover[i] = 1; nCover++; if (solid(x, y)) nSolid++; else if (inEA(x, y)) nOwn++; else if (alpha(x, y) > 0.0f) nEdge++; }
        else if (d) nDiffOut++;
    }
    size_t planted = 0; for (int k = 0; k < 4; ++k) planted += cover[(size_t)PY * W + PX[k]];
    // the alpha the two outputs wrote
    const uint16_t hOne = ToHalf(1.0f);
    size_t a0Miss = 0, a1Miss = 0, c0Alpha = 0, eCover = 0, oAlpha = 0, plantedGot = 0;
    for (size_t i = 0; i < (size_t)W * H; ++i)
    {
        const bool z32 = N.c1.f32[4 * i + 3] == 0.0f, z16 = N.c1.f16[4 * i + 3] == 0, o32 = N.c1.f32[4 * i + 3] == 1.0f, o16 = N.c1.f16[4 * i + 3] == hOne;
        if (cover[i]) { if (!(z32 && z16)) a0Miss++; } else if (!(o32 && o16)) a1Miss++;
        if (N.c0.f32[4 * i + 3] != 1.0f || N.c0.f16[4 * i + 3] != hOne) c0Alpha++;
        if (E.c1.f32[4 * i + 3] != 1.0f || E.c1.f16[4 * i + 3] != hOne) eCover++;
        if (O.c1.f32[4 * i + 3] != 1.0f || O.c1.f16[4 * i + 3] != hOne) oAlpha++;
    }
    for (int k = 0; k < 4; ++k) plantedGot += N.c1.f16[4 * ((size_t)PY * W + PX[k]) + 3] == 0 ? 1 : 0;
    auto onCover = [&](int x, int y) { return cover[(size_t)y * W + x] != 0 && !InHud(x, y); };
    auto offBoth = [&](int x, int y) { return !cover[(size_t)y * W + x] && !InHud(x, y); };
    auto patch = [&](int x, int y) { return x >= CX0 && x < CX1 && y >= CY0 && y < CY1; };
    auto onHud = [](int x, int y) { return InHud(x, y); };
    size_t full = 0, part = 0, none = 0, hudNew = 0, hudOld = 0, nHud = 0, ctlSame = 0, ctlZero = 0, eSame = 0; float leastFactor = 1.0f;
    for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
    {
        const size_t i = (size_t)y * W + x;
        if (InHud(x, y)) { nHud++; hudNew += PixelDiffers(N.c0, N.c1, i) ? 1 : 0; hudOld += PixelDiffers(O.c0, O.c1, i) ? 1 : 0; continue; }
        if (cover[i]) { const float f = factor(N.c1, N.c0, i); leastFactor = std::min(leastFactor, f); if (f >= 1.0f) full++; else if (f > 0.0f) part++; else none++; }
        if (solid(x, y)) { ctlSame += PixelDiffers(O.c0, O.c1, i) ? 0 : 1; ctlZero += factor(O.c1, O.c0, i) == 0.0f ? 1 : 0; eSame += PixelDiffers(E.c0, E.c1, i) ? 0 : 1; }
    }
    const Cmp cScene = Compare(v, R, N.c1, onCover), cOff = Compare(v, N.c0, N.c1, offBoth), cPatch = Compare(v, N.c0, N.c1, patch), oOff = Compare(v, O.c0, O.c1, offBoth), oScene = Compare(v, R, O.c1, solid);
    printf("  THE PHONE: a scene, a phone blended over it at %d,%d (%d by %d, a soft edge of %.0f pixels, its screen lit) in a viewport of %d by %d, then the HUD; four single-step pixels planted in the viewport, a patch of %d pixels outside it; plain, no sharpening\n",
           X0, Y0, X1 - X0, Y1 - Y0, RAMP, VX1 - VX0, VY1 - VY0, (CX1 - CX0) * (CY1 - CY0));
    Proof(a0Miss == 0 && a1Miss == 0 && nCover > 60000 && planted == 4 && plantedGot == 4 && nDiffOut == (size_t)(CX1 - CX0) * (CY1 - CY0) && nOwn == (size_t)EN * EN && nSolid + nEdge + nOwn + planted == nCover,
          "p  THE COVER IS EXACT: the two copies differ (FP16 bits, any of the four channels) on %zu pixels inside the phone's viewport (%zu the phone covers in full with another colour than the scene's, %zu of its soft edge, %zu of its own colour by the fourth channel alone, the 4 planted single steps: %zu of them) and on %zu outside it;"
          " the second output's alpha is 0 on every pixel of the first kind (%zu are not) and 1 on every other pixel of the frame (%zu are not); the planted single steps (values %.3g, 0.5, 4, 60) read alpha 0 on %zu of 4",
          nCover, nSolid, nEdge, nOwn, planted, nDiffOut, a0Miss, a1Miss, (double)PV[0], plantedGot);
    {   // the two blocks of the phone's own colour
        size_t eaCover = 0, eaColour = 0, ebAlpha1 = 0, ebOne = 0, ebColour = 0;
        for (int y = EY; y < EY + EN; ++y) for (int x = EAX; x < EBX + EN; ++x)
        {
            const size_t i = (size_t)y * W + x;
            const bool colour = ToHalf(S[i].r) != ToHalf(FP[i].r) || ToHalf(S[i].g) != ToHalf(FP[i].g) || ToHalf(S[i].b) != ToHalf(FP[i].b);
            if (inEA(x, y)) { eaColour += colour ? 1 : 0; eaCover += (N.c1.f32[4 * i + 3] == 0.0f && N.c1.f16[4 * i + 3] == 0) ? 1 : 0; }
            if (inEB(x, y)) { ebColour += colour ? 1 : 0; ebAlpha1 += (N.c1.f32[4 * i + 3] == 1.0f && N.c1.f16[4 * i + 3] == hOne) ? 1 : 0; ebOne += PixelDiffers(N.c0, N.c1, i) ? 0 : 1; }
        }
        Proof(eaColour == 0 && ebColour == 0 && eaCover == (size_t)EN * EN && ebAlpha1 == (size_t)EN * EN && ebOne == (size_t)EN * EN,
              "p  WHERE THE PHONE'S COLOUR IS THE SCENE'S BEHIND IT (two blocks of %d pixels inside the part it covers in full; the three colour channels of the two copies differ on %zu and %zu of them):"
              " where the phone's draw moved the back buffer's FOURTH channel the cover says the pixel (alpha 0 on %zu of %d); where no channel differs no copy can say it: alpha 1 on %zu, and the two pictures are one on %zu (scene to both: the limit, said)",
              EN * EN, eaColour, ebColour, eaCover, EN * EN, ebAlpha1, ebOne);
    }
    Proof(cScene.maxUlp <= 64 && cScene.nanMismatch == 0 && cScene.maxHalfStep <= 1 && cScene.n == nCover,
          "p  ON THE COVER the HUD-less picture is the shipping encode of the frame BEFORE the phone (%s). AMD's own line on those pixels: the real pixel in full on %zu, by its share on %zu, not at all on %zu (the least factor %.3f): where it is under 1 the stock rule shows interpolated scene, and the mod's library reads the alpha instead",
          Tell(cScene), full, part, none, leastFactor);
    Proof(cOff.diff32 == 0 && cOff.diff16 == 0 && cPatch.diff32 == 0 && cPatch.n == (size_t)(CX1 - CX0) * (CY1 - CY0) && oOff.diff32 == 0 && hudNew * 100 >= nHud * 99 && hudOld * 100 >= nHud * 99,
          "p  off the cover and off the HUD the two pictures are one, bit for bit (%zu pixels, %zu differ; the %zu of the patch outside the viewport among them: scene to both), and the HUD is taken out of the HUD-less picture by the rule it has (%zu of its %zu pixels; with the encode's copy alone %zu)",
          cOff.n, cOff.diff32, cPatch.n, hudNew, nHud, hudOld);
    Proof(eCover == 0 && eSame == nSolid && nSolid > 30000,
          "   control, AN EMPTY RECT: no pixel's alpha is 0 (%zu are), and the phone stands in the HUD-less picture bit for bit on %zu of the %zu pixels it covers in full", eCover, eSame, nSolid);
    Proof(ctlSame == nSolid && ctlZero == nSolid && oAlpha == 0 && oScene.diff32 * 100 >= nSolid * 99 && oScene.maxUlp > 64,
          "   control, THE ENCODE'S OWN COPY ALONE (at the first sprite, behind the phone; the encode every other frame draws with): the HUD-less picture holds the PHONE bit for bit on %zu of its %zu pixels, AMD's factor is 0 on %zu (the generated frame shows what was interpolated there),"
          " its alpha is 1 everywhere (%zu are not), and \"the frame before the phone\" FAILS on %zu of them (%lld steps at the most)",
          ctlSame, nSolid, ctlZero, oAlpha, oScene.diff32, (long long)oScene.maxUlp);
    // q  THE REAL PICTURE OF A FRAME WITH BOTH COPIES IS A RELEASE'S, at two UI levels
    {
        auto everyPx = [](int, int) { return true; };
        const Out Rel = Render2(v, enc, c, txOld), RelLow = Render2(v, enc, cLow, txOld), WasLow = Render2(v, enc, cLow, txWas);
        const Cmp q1 = Compare(v, Rel, N.c0, everyPx), q2 = Compare(v, RelLow, Nlow.c0, everyPx);
        const std::string t1 = Tell(q1), t2 = Tell(q2);
        Proof(q1.maxUlp <= 64 && q1.nanMismatch == 0 && q1.maxHalfStep <= 1 && q2.maxUlp <= 64 && q2.nanMismatch == 0 && q2.maxHalfStep <= 1 && c0Alpha == 0,
              "q  THE REAL PICTURE OF A FRAME WITH THE PHONE IS A RELEASE'S: target 0 of the overlay's encode against the shipping encode with the release's copy (at the first sprite), every pixel (the limit 64 steps): at the UI level 1: %s; at 0.15: %s; its alpha is 1 on every pixel (%zu are not)",
              t1.c_str(), t2.c_str(), c0Alpha);
        // the control: fault 1 as it stood, and the release's own zero
        size_t wasDiff = 0, relMoved = 0, hudMoved = 0, nHudPx = 0, nSol = 0;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
        {
            const size_t i = (size_t)y * W + x;
            if (InHud(x, y)) { nHudPx++; hudMoved += PixelDiffers(Rel, RelLow, i) ? 1 : 0; continue; }
            if (!solid(x, y)) continue;
            nSol++;
            wasDiff += PixelDiffers(WasLow, RelLow, i) ? 1 : 0;
            relMoved += PixelDiffers(Rel, RelLow, i) ? 1 : 0;
        }
        Proof(wasDiff == nSol && relMoved == 0 && hudMoved * 100 >= nHudPx * 99,
              "   control, FAULT 1 AS IT STOOD: the shipping encode with the copy AHEAD of the phone as its own (the first form) differs from a release's on %zu of the phone's %zu pixels at the UI level 0.15 (its coverage is above zero there);"
              " with the release's copy the level changes %zu of them (its coverage is zero there), and %zu of the HUD's %zu (the level shows where there is coverage)",
              wasDiff, nSol, relMoved, hudMoved, nHudPx);
    }
    // s  THE GHOSTING FIX STANDS UNDER THE COVER: off the cover the HUD-less picture is the first form's on every pixel, at two UI levels
    //    (the level is the real picture's alone); on the cover what the first form held is held still, and what it left in
    //    both pictures (the dead zone) is counted
    {
        // (off the cover AND where the two copies are one: the patch outside the viewport, which differs between the copy points and is no
        // overlay's, is the one place the two forms part by design: the first form took it out of the HUD-less picture as
        // it took the phone, the cover's form leaves it scene to both pictures, proof p's third line. Counted below.)
        auto offCover = [&](int x, int y) { const size_t i = (size_t)y * W + x; return !cover[i] && !differs[i]; };
        const Cmp sPatch = Compare(v, N.c0, N.c1, patch);
        const Out2 Was = RenderMrt(v, mrtPs, c, txWas, rt32b, rt16b), WasLow = RenderMrt(v, mrtPs, cLow, txWas, rt32b, rt16b);
        const Cmp wPatch = Compare(v, Was.c0, Was.c1, patch);
        std::vector<uint8_t> wasHeld((size_t)W * H, 0); size_t nHeld = 0, nLeft = 0;
        for (size_t i = 0; i < (size_t)W * H; ++i) if (cover[i]) { if (PixelDiffers(Was.c0, Was.c1, i)) { wasHeld[i] = 1; nHeld++; } else nLeft++; }
        auto held = [&](int x, int y) { return wasHeld[(size_t)y * W + x] != 0; };
        auto left = [&](int x, int y) { const size_t i = (size_t)y * W + x; return cover[i] && !wasHeld[i]; };
        const Cmp s1 = Compare(v, Was.c1, N.c1, offCover), s2 = Compare(v, WasLow.c1, Nlow.c1, offCover), s3 = Compare(v, Was.c1, N.c1, held), s4 = Compare(v, R, N.c1, left);
        const Cmp sc = Compare(v, O.c1, N.c1, offCover), sd = Compare(v, O.c1, N.c1, solid);
        const std::string t1 = Tell(s1), t2 = Tell(s2), t3 = Tell(s3), t4 = Tell(s4);
        Proof(s1.maxUlp <= 64 && s1.nanMismatch == 0 && s1.maxHalfStep <= 1 && s2.maxUlp <= 64 && s2.nanMismatch == 0 && s2.maxHalfStep <= 1 && a1Miss == 0
              && s3.maxUlp <= 64 && s3.nanMismatch == 0 && s3.maxHalfStep <= 1 && s3.n == nHeld && nHeld > 30000
              && s4.maxUlp <= 64 && s4.nanMismatch == 0 && s4.maxHalfStep <= 1 && s4.n == nLeft && nLeft > 0 && nHeld + nLeft == nCover
              && s1.n + nCover + nDiffOut == (size_t)W * H && sPatch.diff32 == 0,
              "s  THE GHOSTING FIX STANDS UNDER THE COVER: target 1 of the overlay's encode against target 1 of the first form (eca6ac9b's, the ghosting fix as the owner saw it; the limit 64 steps): OFF THE COVER, every pixel where the two copies are one, at the UI level 1: %s; at 0.15: %s; its alpha is 1 there (%zu are not);"
              " ON THE COVER, the %zu pixels where the first form held the frame before the phone: %s; and the %zu it LEFT IN BOTH PICTURES (the mask's dead zone, the see-through) now hold the frame before the phone: %s."
              " THE ONE PLACE THE TWO FORMS PART: the %zu pixels of the patch outside the viewport (something else drawn between the copy points): the first form took %zu of them out of the HUD-less picture, the cover's form leaves them scene to both (%zu differ)",
              t1.c_str(), t2.c_str(), a1Miss, nHeld, t3.c_str(), nLeft, t4.c_str(), nDiffOut, wPatch.diff32, sPatch.diff32);
        Proof(sc.maxUlp <= 64 && sc.maxHalfStep <= 1 && sd.diff32 * 100 >= sd.n * 99 && sd.maxUlp > 64,
              "   control: against the HUD-less picture made from the encode's own copy alone (the phone in it) the compare off the cover holds (%zu of %zu differ) and on the phone it FAILS on %zu of %zu pixels (%lld steps at the most)",
              sc.diff32, sc.n, sd.diff32, sd.n, (long long)sd.maxUlp);
    }
    // r  THE OVERLAY'S COPY ALONE: the HUD off, the frame ends with the phone, the encode has no copy of its own (its mask off)
    {
        auto everyPx = [](int, int) { return true; };
        auto notDiff = [&](int x, int y) { return !differs[(size_t)y * W + x]; };
        const Out2 A = RenderMrt(v, mrtPs, cNo, txOff, rt32b, rt16b);
        const Out RelOff = Render2(v, enc, cNo, txOffR);
        const Cmp r0 = Compare(v, RelOff, A.c0, everyPx), r1 = Compare(v, R, A.c1, solid), rOut = Compare(v, A.c0, A.c1, notDiff);
        size_t fullA = 0, nSol = 0, wrote = 0, under = 0, aAlpha = 0;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
        {
            const size_t i = (size_t)y * W + x;
            if (A.c1.f32[4 * i + 3] != 1.0f || A.c1.f16[4 * i + 3] != hOne) aAlpha++;
            if (solid(x, y)) { nSol++; fullA += factor(A.c1, A.c0, i) >= 1.0f ? 1 : 0; }
            if (cover[i] && !solid(x, y)) { if (PixelDiffers(A.c0, A.c1, i)) wrote++; else under++; }
        }
        const std::string t0 = Tell(r0), t1 = Tell(r1);
        Proof(r0.maxUlp <= 64 && r0.nanMismatch == 0 && r0.maxHalfStep <= 1 && r1.maxUlp <= 64 && r1.nanMismatch == 0 && r1.maxHalfStep <= 1 && rOut.diff32 == 0 && rOut.diff16 == 0 && fullA == nSol && aAlpha == 0 && under > 0,
              "r  THE OVERLAY'S COPY ALONE (the game's HUD off): target 0 is the shipping encode's of that frame, its mask off (%s); on the %zu pixels the phone covers in full target 1 is the shipping encode of the frame before the phone (%s), AMD's line gives the real pixel in full on %zu;"
              " where the two frames do not differ the two pictures are one (%zu differ); its alpha is 1 everywhere (%zu are not). NOT EXACT ON SUCH A FRAME: of the cover's other pixels (its soft edge, the single steps) %zu hold the frame before the phone and %zu stand in both pictures (the mask's dead zone)",
              t0.c_str(), r1.n, t1.c_str(), fullA, rOut.diff32, aAlpha, wrote, under);
    }
    // THE COVER'S ALPHA OVER THE WIRE'S FIRST LEG: the phone's frame through the worker's raw copy, by the product's own lines (proof h's sequence)
    OvRect(v, VX0, VY0, VX1, VY1);
    RawProof(v, mrtOvPs, rt16b, mode, txNew, "p  THE COVER'S ALPHA IN THE RAW COPY:");
    tS->Release(); tFP->Release(); tF->Release();
}
// ================================================================================================================
// SDR (2026-10-20; source/fg/fg_hud.h, "SDR"; source/fg/fg_hud_rule.h, the SDR rules). THE HUD-LESS PICTURE OF AN 8-BIT BACK
// BUFFER, drawn here in the game's own order through DXVK, in both 8-bit formats a back buffer can have (X8R8G8B8, A8R8G8B8):
// the frame (the game's final pass) in the back buffer; AT THE INTERFACE'S FIRST DRAW the product's copy (into the second
// target, or, where FusionFix's console gamma is on, into a copy of its own); the interface (blended 2D sprites); WHERE THE
// CURVE IS ON, FusionFix's pass as FusionFix makes it at its EndScene (a point copy of the back buffer, then one draw through
// its curve and dither: the mod's own reproduction, hdr_cgablit_ps.hlsl, stands for FusionFix's shader) AND THAT DRAW A SECOND
// TIME, sampler 0 the product's copy and the second target its target, every other state as it stood; then a notice of the
// mod's own (hkEndScene, behind the curve).
//   u  THE HUD-LESS PICTURE HOLDS NO INTERFACE: it is THE FRAME WITHOUT THE INTERFACE (the same frame drawn with no sprite and no
//      notice, the curve on it where it is on) on EVERY pixel, bit for bit, and so the presented picture on every pixel the
//      interface and the notice left alone; the presented picture differs from it on 99 percent or more of the interface's
//      pixels and on the notice's: AMD's interpolation keeps the full picture's interface over it (computeInpainting).
//      CONTROLS, which must fail: THE COPY POINT BEHIND THE INTERFACE (taken after the last sprite): the HUD-less picture holds the
//      interface; THE CURVE NOT DRAWN A SECOND TIME (the copy straight into the second target with the curve on): the two
//      pictures differ off the interface (the product's rule hands such a frame with no HUD-less picture: FgHudSdrPicRule).
//   v  THE WORKER'S COPY OUT OF THE SECOND TARGET BY THE PRODUCT'S OWN LINES, in proof h's order, the target written by the copy
//      (no curve) and by the curve's second draw: behind the flush the raw image holds the frame's HUD-less picture bit for bit,
//      and the next frame's write into the target meets the image where the copy left it. CONTROL: the next frame's picture.
//   --time: THE COST at 3840x2160 and 1920x1080, X8R8G8B8: whole frames as the game orders them, 300 back to back, with and
//      without the product's work, the curve on and off; the product's cost is its frame less the same frame without it.
//     hudless.exe --d3d9 <path> --sdr [--time] [--layouts]   these alone (the default run has them too, at its end)
static bool SdrNotice(int x, int y) { return x >= W - 160 && y >= H - 48; }
struct Sdr
{
    int w, h; D3DFORMAT fmt; IDirect3DPixelShader9* curve;
    Dev dv;                              // (the device, its vertex shader and this size alone: Draw's; NOT a copy of v, whose two frames are megabytes a copy)
    IDirect3DTexture9 *src, *bb, *T, *pre, *t2, *ref;
    IDirect3DSurface9 *srcS, *bbS, *TS, *preS, *t2S, *refS, *sys;
};
static void SdrMake(Sdr& s, Dev& v, int w, int h, D3DFORMAT fmt, IDirect3DPixelShader9* curve, uint32_t seed)
{
    s = {}; s.w = w; s.h = h; s.fmt = fmt; s.curve = curve;
    s.dv.d = v.d; s.dv.vs = v.vs; s.dv.decl = v.decl; s.dv.w = w; s.dv.h = h;
    IDirect3DTexture9** tx[6] = { &s.src, &s.bb, &s.T, &s.pre, &s.t2, &s.ref };
    IDirect3DSurface9** sf[6] = { &s.srcS, &s.bbS, &s.TS, &s.preS, &s.t2S, &s.refS };
    for (int i = 0; i < 6; ++i) { CK(v.d->CreateTexture(w, h, 1, D3DUSAGE_RENDERTARGET, fmt, D3DPOOL_DEFAULT, tx[i], nullptr)); CK((*tx[i])->GetSurfaceLevel(0, sf[i])); }
    CK(v.d->CreateOffscreenPlainSurface(w, h, fmt, D3DPOOL_SYSTEMMEM, &s.sys, nullptr));
    // the frame: gradients, a checker of flat patches, noise; every byte of it, the fourth too (the game's back buffer holds values there)
    D3DLOCKED_RECT lr; CK(s.sys->LockRect(&lr, nullptr, 0));
    uint32_t r = seed;
    for (int y = 0; y < h; ++y)
    {
        BYTE* row = (BYTE*)lr.pBits + (size_t)y * lr.Pitch;
        for (int x = 0; x < w; ++x)
        {
            r ^= r << 13; r ^= r >> 17; r ^= r << 5;
            BYTE* p = row + 4 * x;
            if (y < h / 4) { p[0] = (BYTE)(x * 255 / (w - 1)); p[1] = (BYTE)(y * 255 / (h / 4)); p[2] = 128; }
            else if (y < h / 2) { p[0] = p[1] = p[2] = (BYTE)(((x / 32 + y / 32) & 1) ? 200 : 30); }
            else { p[0] = (BYTE)r; p[1] = (BYTE)(r >> 8); p[2] = (BYTE)(r >> 16); }
            p[3] = (BYTE)(r >> 24);
        }
    }
    s.sys->UnlockRect();
    CK(v.d->UpdateSurface(s.sys, nullptr, s.srcS, nullptr));
}
static void SdrFree(Sdr& s)
{
    IUnknown* all[13] = { s.srcS, s.bbS, s.TS, s.preS, s.t2S, s.refS, s.sys, s.src, s.bb, s.T, s.pre, s.t2, s.ref };
    for (IUnknown* u : all) if (u) u->Release();
    s = {};
}
// A BLENDED 2D SPRITE (the interface, the notice; a band of the scene on frame B): pre-transformed, by the fixed function, as the game's
// 2D pass draws its sprites. The rect's pixels x0..x1-1, y0..y1-1.
static void SdrQuad(Dev& v, IDirect3DSurface9* rt, int w, int h, int x0, int y0, int x1, int y1, D3DCOLOR c)
{
    IDirect3DDevice9* d = v.d;
    CK(d->SetRenderTarget(0, rt));
    D3DVIEWPORT9 vp = { 0, 0, (DWORD)w, (DWORD)h, 0.0f, 1.0f }; d->SetViewport(&vp);
    d->SetVertexShader(nullptr); d->SetPixelShader(nullptr); d->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE); d->SetTexture(0, nullptr);
    d->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1); d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1); d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
    d->SetRenderState(D3DRS_LIGHTING, FALSE); d->SetRenderState(D3DRS_ZENABLE, FALSE); d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE); d->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA); d->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    struct Vx { float x, y, z, rhw; DWORD c; };
    const float a = (float)x0 - 0.5f, b = (float)y0 - 0.5f, e = (float)x1 - 0.5f, f = (float)y1 - 0.5f;
    const Vx q[4] = { { a, b, 0, 1, c }, { e, b, 0, 1, c }, { a, f, 0, 1, c }, { e, f, 0, 1, c } };
    CK(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, q, sizeof(Vx)));
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}
// FusionFix's pass, or its second draw: sampler 0 `in`, the target `out`, the curve's constants (hdr_cgablit_ps.hlsl: c0 the size, c7
// the curve, the dither and FusionFix's c44, the screen's size)
static void SdrCurve(Sdr& s, Dev& v, IDirect3DTexture9* in, IDirect3DTexture9* out, float curve, float dither)
{
    float c[9][4] = {}; c[0][0] = (float)s.w; c[0][1] = (float)s.h; c[0][2] = 1.0f / s.w; c[0][3] = 1.0f / s.h;
    c[7][0] = curve; c[7][1] = dither; c[7][2] = (float)s.w; c[7][3] = (float)s.h;
    IDirect3DTexture9* tx[12] = {}; tx[0] = in;
    (void)v; Draw(s.dv, s.curve, c, out, tx);
}
// FUSIONFIX'S DRAW AS FUSIONFIX MAKES IT, STATE FIRST (g10.cpp's Draw, line for line, without its draw), and, where `pre` is given, THE
// MOD'S OWN SECOND DRAW AHEAD OF IT: source\fg\fg_hud_sdr_d3d.h's FgHudSdrReplayD3D, the function the mod calls, with the same draw.
// vpMinZ and scissor: a viewport depth range and a scissor rect FusionFix's state might hold (proof w reads them back).
static const D3DVIEWPORT9* gSdrVp = nullptr; static const RECT* gSdrSc = nullptr;
static void SdrCurveFF(Sdr& s, IDirect3DTexture9* in, IDirect3DTexture9* out, float curve, float dither, IDirect3DTexture9* pre, bool naive = false)
{
    IDirect3DDevice9* d = s.dv.d;
    float c[9][4] = {}; c[0][0] = (float)s.w; c[0][1] = (float)s.h; c[0][2] = 1.0f / s.w; c[0][3] = 1.0f / s.h;
    c[7][0] = curve; c[7][1] = dither; c[7][2] = (float)s.w; c[7][3] = (float)s.h;
    IDirect3DSurface9* outS = nullptr; CK(out->GetSurfaceLevel(0, &outS));
    CK(d->SetRenderTarget(0, outS));
    D3DVIEWPORT9 vp = { 0, 0, (DWORD)s.w, (DWORD)s.h, 0.0f, 1.0f }; if (gSdrVp) vp = *gSdrVp; d->SetViewport(&vp);
    if (gSdrSc) d->SetScissorRect(gSdrSc);
    d->SetRenderState(D3DRS_ZENABLE, FALSE); d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); d->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF); d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
    for (int i = 0; i < 12; ++i) d->SetTexture(i, i == 0 ? in : nullptr);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT); d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT); d->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    d->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP); d->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP); d->SetSamplerState(0, D3DSAMP_SRGBTEXTURE, FALSE);
    d->SetVertexShader(s.dv.vs); d->SetVertexDeclaration(s.dv.decl); d->SetPixelShader(s.curve);
    d->SetPixelShaderConstantF(0, &c[0][0], 9);
    auto draw = [&]() { return d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kIvtQuad, sizeof(kIvtQuad[0])); };
    if (pre)
    {
        IDirect3DSurface9* t2S = nullptr; CK(s.t2->GetSurfaceLevel(0, &t2S));
        if (!naive) CK(FgHudSdrReplayD3D(d, t2S, outS, pre, in, draw));
        else { d->SetRenderTarget(0, t2S); d->SetTexture(0, pre); CK(draw()); d->SetTexture(0, in); d->SetRenderTarget(0, outS); }   // (proof w's control: no viewport or scissor put back)
        t2S->Release();
    }
    CK(draw());
    outS->Release();
}
// THE INTERFACE: a panel over the interface's rect and a batch of thin bars in it (text-like), all blended
static void SdrInterface(Sdr& s, Dev& v, IDirect3DSurface9* bb)
{
    const int sx = s.w / W ? s.w / W : 1, sy = s.h / H ? s.h / H : 1;
    SdrQuad(v, bb, s.w, s.h, 0, 128 * sy, (W / 4) * sx, 256 * sy, 0x99203040);
    for (int i = 0; i < 16; ++i) SdrQuad(v, bb, s.w, s.h, (8 + 14 * i) * sx, (140 + (i & 3) * 28) * sy, (16 + 14 * i) * sx, (160 + (i & 3) * 28) * sy, 0xE0FFE080);
}
// ONE FRAME IN THE GAME'S ORDER, by mode: the product; the copy point behind the interface (a control); the curve not drawn a second
// time (a control); the reference (the same frame with no interface and no notice, the curve on it where it is on, into `ref`).
// k: frame B carries a band of the scene of its own, drawn ahead of the copy point (proof v's next frame).
enum { SDR_PRODUCT = 0, SDR_LATE = 1, SDR_NOREPLAY = 2, SDR_REF = 3 };
static void SdrFrame(Sdr& s, Dev& v, float curve, float dither, int mode, int k)
{
    IDirect3DDevice9* d = v.d;
    const bool on = curve != 0.0f;
    IDirect3DSurface9* const bb = mode == SDR_REF ? s.refS : s.bbS;
    IDirect3DTexture9* const bbT = mode == SDR_REF ? s.ref : s.bb;
    CK(d->StretchRect(s.srcS, nullptr, bb, nullptr, D3DTEXF_NONE));                        // the game's final pass: the frame
    if (k) SdrQuad(v, bb, s.w, s.h, 0, 0, s.w, s.h / 8, 0x80FF00FF);                        // (frame B: scene, ahead of the copy point)
    const bool intoPre = on && mode == SDR_PRODUCT;                                           // (FgHudSdrIntoPreRule: the curve latched on)
    if (mode == SDR_PRODUCT || mode == SDR_NOREPLAY)
        CK(d->StretchRect(bb, nullptr, intoPre ? s.preS : s.t2S, nullptr, D3DTEXF_NONE));    // THE COPY POINT: the interface's first draw (FgHudSdrCopy)
    if (mode != SDR_REF) SdrInterface(s, v, bb);
    if (mode == SDR_LATE) CK(d->StretchRect(bb, nullptr, s.t2S, nullptr, D3DTEXF_NONE));     // THE PLANTED WRONG COPY POINT: behind the interface
    if (on)
    {
        CK(d->StretchRect(bb, nullptr, s.TS, nullptr, D3DTEXF_POINT));                       // FusionFix's point copy of the back buffer...
        SdrCurveFF(s, s.T, bbT, curve, dither, intoPre ? s.pre : nullptr);                    // ...its draw through the curve, and AHEAD OF IT the mod's second draw (FgHudSdrReplayD3D)
    }
    if (mode != SDR_REF) SdrQuad(v, bb, s.w, s.h, s.w - 160, s.h - 48, s.w, s.h, 0xC0000000);   // the mod's notice
}
static std::vector<uint8_t> SdrRead(Sdr& s, Dev& v, IDirect3DSurface9* surf)
{
    CK(v.d->GetRenderTargetData(surf, s.sys));
    std::vector<uint8_t> out((size_t)s.w * s.h * 4); D3DLOCKED_RECT lr; CK(s.sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
    for (int y = 0; y < s.h; ++y) memcpy(&out[(size_t)y * s.w * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, (size_t)s.w * 4);
    s.sys->UnlockRect(); return out;
}
static bool SdrRgbDiffers(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b, size_t i) { return memcmp(&a[4 * i], &b[4 * i], 3) != 0; }
static const char* SdrFmtName(D3DFORMAT f) { return f == D3DFMT_X8R8G8B8 ? "X8R8G8B8" : "A8R8G8B8"; }

static void SdrProofU(Dev& v, IDirect3DPixelShader9* cps)
{
    struct Cv { const char* name; float curve, dither; };
    const Cv cs[4] = { { "no console gamma", 0.0f, 0.0f }, { "console gamma, the Xbox curve, dither on", 1.0f, 1.0f },
                       { "console gamma, the Xbox curve, dither off", 1.0f, 0.0f }, { "console gamma, the PS3 curve, dither on", 2.0f, 1.0f } };
    size_t nHud = 0, nNot = 0; for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) { nHud += InHud(x, y) ? 1 : 0; nNot += SdrNotice(x, y) ? 1 : 0; }
    for (const D3DFORMAT f : { D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8 }) for (const Cv& c : cs)
    {
        Sdr s; SdrMake(s, v, W, H, f, cps, 0x9E3779B9u);
        SdrFrame(s, v, c.curve, c.dither, SDR_REF, 0);
        const std::vector<uint8_t> R = SdrRead(s, v, s.refS);
        SdrFrame(s, v, c.curve, c.dither, SDR_PRODUCT, 0);
        const std::vector<uint8_t> P = SdrRead(s, v, s.bbS), Hh = SdrRead(s, v, s.t2S);
        size_t hNotR = 0, hNotPOff = 0, hudDiff = 0, notDiff = 0, refHud = 0;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x)
        {
            const size_t i = (size_t)y * W + x; const bool hud = InHud(x, y), nt = SdrNotice(x, y);
            hNotR += SdrRgbDiffers(Hh, R, i) ? 1 : 0;
            if (!hud && !nt) hNotPOff += SdrRgbDiffers(Hh, P, i) ? 1 : 0;
            if (hud) { hudDiff += SdrRgbDiffers(Hh, P, i) ? 1 : 0; refHud += SdrRgbDiffers(R, P, i) ? 1 : 0; }
            if (nt) notDiff += SdrRgbDiffers(Hh, P, i) ? 1 : 0;
        }
        printf("  SDR, %s, %s\n", SdrFmtName(f), c.name);
        Proof(hNotR == 0 && hNotPOff == 0 && hudDiff * 100 >= nHud * 99 && notDiff * 100 >= nNot * 99,
              "u  the HUD-less picture is the frame without the interface on every pixel (%zu of %zu differ) and the presented picture off the interface and the notice (%zu differ);"
              " the presented picture differs from it on %zu of the interface's %zu pixels and %zu of the notice's %zu", hNotR, (size_t)W * H, hNotPOff, hudDiff, nHud, notDiff, nNot);
        // CONTROL: the copy point behind the interface
        SdrFrame(s, v, c.curve, c.dither, SDR_LATE, 0);
        const std::vector<uint8_t> Pl = SdrRead(s, v, s.bbS), Hl = SdrRead(s, v, s.t2S);
        size_t lateHud = 0; for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) if (InHud(x, y)) lateHud += SdrRgbDiffers(Hl, R, (size_t)y * W + x) ? 1 : 0;
        Proof(lateHud * 100 >= nHud * 99, "   control: the copy point behind the interface: the HUD-less picture holds the interface (%zu of its %zu pixels are not the frame without it)", lateHud, nHud);
        (void)Pl;
        if (c.curve != 0.0f)
        {   // CONTROL: the curve not drawn a second time
            SdrFrame(s, v, c.curve, c.dither, SDR_NOREPLAY, 0);
            const std::vector<uint8_t> Pn = SdrRead(s, v, s.bbS), Hn = SdrRead(s, v, s.t2S);
            size_t off = 0, nOff = 0;
            for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) if (!InHud(x, y) && !SdrNotice(x, y)) { nOff++; off += SdrRgbDiffers(Hn, Pn, (size_t)y * W + x) ? 1 : 0; }
            Proof(off * 2 >= nOff, "   control: the curve not drawn a second time: the two pictures differ on %zu of the %zu pixels off the interface", off, nOff);
        }
        SdrFree(s);
    }
}
// v  (proof h's lines and order, for the SDR target: RawProof says each step)
static void SdrProofV(Dev& v, IDirect3DPixelShader9* cps, const char* mode)
{
    ID3D9VkInteropDevice* idev = nullptr;
    if (FAILED(v.d->QueryInterface(__uuidof(ID3D9VkInteropDevice), (void**)&idev)) || !idev) { Proof(false, "v  the device answers no ID3D9VkInteropDevice: it is not DXVK's"); return; }
    VkInstance inst = nullptr; VkPhysicalDevice phys = nullptr; VkDevice dev = nullptr; idev->GetVulkanHandles(&inst, &phys, &dev);
    VkQueue q = nullptr; uint32_t qi = 0, fam = 0; idev->GetSubmissionQueue(&q, &qi, &fam);
    FeedVk vk; const char* miss = nullptr;
    if (!dev || !q || !FeedVkLoad(&vk, dev, &miss)) { Proof(false, "v  the transport's loader did not take its entry points (%s)", miss ? miss : "no device or queue"); idev->Release(); return; }
#define GDP(n) const PFN_##n n##_ = (PFN_##n)vk.GetDeviceProcAddr(dev, #n)
    GDP(vkCreateCommandPool); GDP(vkDestroyCommandPool); GDP(vkAllocateCommandBuffers); GDP(vkBeginCommandBuffer); GDP(vkEndCommandBuffer); GDP(vkQueueSubmit);
    GDP(vkCreateFence); GDP(vkDestroyFence); GDP(vkWaitForFences);
#undef GDP
    const HMODULE vl = GetModuleHandleA("vulkan-1.dll");
    const PFN_vkGetInstanceProcAddr gipa = vl ? (PFN_vkGetInstanceProcAddr)GetProcAddress(vl, "vkGetInstanceProcAddr") : nullptr;
    const PFN_vkGetPhysicalDeviceMemoryProperties gmp = gipa ? (PFN_vkGetPhysicalDeviceMemoryProperties)gipa(inst, "vkGetPhysicalDeviceMemoryProperties") : nullptr;
    if (!vkCreateCommandPool_ || !vkAllocateCommandBuffers_ || !vkBeginCommandBuffer_ || !vkEndCommandBuffer_ || !vkQueueSubmit_ || !vkCreateFence_ || !vkWaitForFences_ || !gmp)
    { Proof(false, "v  a Vulkan entry point this proof needs is missing"); idev->Release(); return; }
    VkPhysicalDeviceMemoryProperties mp = {}; gmp(phys, &mp);
    for (const float curve : { 0.0f, 1.0f })
    {
        Sdr s; SdrMake(s, v, W, H, D3DFMT_X8R8G8B8, cps, 0x2545F491u);
        SdrFrame(s, v, curve, 1.0f, SDR_PRODUCT, 0); const std::vector<uint8_t> refA = SdrRead(s, v, s.t2S);
        SdrFrame(s, v, curve, 1.0f, SDR_PRODUCT, 1); const std::vector<uint8_t> refB = SdrRead(s, v, s.t2S);
        CK(v.d->ColorFill(s.t2S, nullptr, 0)); CK(v.d->ColorFill(s.refS, nullptr, 0));
        // the raw image (standing for the helper's HUDLESS), in the target's own Vulkan format
        idev->FlushRenderingCommands();
        VkImage tImg = VK_NULL_HANDLE; VkImageLayout tLay = VK_IMAGE_LAYOUT_UNDEFINED; UINT tw = 0, th = 0; VkFormat tf = VK_FORMAT_UNDEFINED;
        const bool q0 = FgvBbQuery(s.t2S, &tImg, &tLay, &tw, &th, &tf);
        FgvImage X = {};
        VkCommandPool pool = VK_NULL_HANDLE; VkCommandBuffer cb[2] = {}; VkFence fence[2] = {};
        VkCommandPoolCreateInfo pci = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO }; pci.queueFamilyIndex = fam;
        VkFenceCreateInfo fci = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
        bool ok = q0 && FgvImageMake(&vk, mp, (UINT)W, (UINT)H, tf, &X) && vkCreateCommandPool_(dev, &pci, nullptr, &pool) == VK_SUCCESS;
        VkCommandBufferAllocateInfo cai = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO }; cai.commandPool = pool; cai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; cai.commandBufferCount = 2;
        ok = ok && vkAllocateCommandBuffers_(dev, &cai, cb) == VK_SUCCESS && vkCreateFence_(dev, &fci, nullptr, &fence[0]) == VK_SUCCESS && vkCreateFence_(dev, &fci, nullptr, &fence[1]) == VK_SUCCESS;
        if (!ok) { Proof(false, "v  this proof's own Vulkan objects could not be made"); SdrFree(s); idev->Release(); return; }
        auto begin = [&](VkCommandBuffer c) { VkCommandBufferBeginInfo bi = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO }; bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; return vkBeginCommandBuffer_(c, &bi) == VK_SUCCESS; };
        auto submit = [&](VkCommandBuffer c, VkFence f) { VkSubmitInfo si = { VK_STRUCTURE_TYPE_SUBMIT_INFO }; si.commandBufferCount = 1; si.pCommandBuffers = &c; return vkEndCommandBuffer_(c) == VK_SUCCESS && vkQueueSubmit_(q, 1, &si, f) == VK_SUCCESS; };
        SdrFrame(s, v, curve, 1.0f, SDR_PRODUCT, 0);              // frame A: the copy (and, the curve on, its second draw) writes the target
        idev->FlushRenderingCommands();                            // the hand's span: the flush
        const bool q1 = FgvBbQuery(s.t2S, &tImg, &tLay, &tw, &th, &tf);
        idev->LockSubmissionQueue();
        bool s1 = begin(cb[0]);
        FeedVkBarrierN(&vk, cb[0], &X.img, 1, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
        const bool rec1 = q1 && FgvRecCopyOut(&vk, cb[0], tImg, tLay, X.img, false, fam, tw, th);
        s1 = s1 && submit(cb[0], fence[0]);
        idev->ReleaseSubmissionQueue();
        SdrFrame(s, v, curve, 1.0f, SDR_PRODUCT, 1);              // frame B, into the target the copy has just read
        idev->FlushRenderingCommands();
        VkImage rImg = VK_NULL_HANDLE; VkImageLayout rLay = VK_IMAGE_LAYOUT_UNDEFINED; UINT rw = 0, rh = 0; VkFormat rf = VK_FORMAT_UNDEFINED;
        const bool q2 = FgvBbQuery(s.refS, &rImg, &rLay, &rw, &rh, &rf);
        idev->LockSubmissionQueue();
        bool s2 = begin(cb[1]);
        const bool rec2 = q2 && FgvRecCopyIn(&vk, cb[1], X.img, false, fam, rImg, rLay, rw, rh);
        s2 = s2 && submit(cb[1], fence[1]);
        idev->ReleaseSubmissionQueue();
        const bool retired = vkWaitForFences_(dev, 2, fence, VK_TRUE, 10000000000ull) == VK_SUCCESS;
        const std::vector<uint8_t> gotR = SdrRead(s, v, s.refS), gotT = SdrRead(s, v, s.t2S);
        size_t nA = 0, nB = 0, ctl = 0, ab = 0; const size_t n = (size_t)W * H;
        for (size_t i = 0; i < n; ++i)
        {
            nA += SdrRgbDiffers(gotR, refA, i) ? 1 : 0; nB += SdrRgbDiffers(gotT, refB, i) ? 1 : 0;
            ctl += SdrRgbDiffers(gotR, refB, i) ? 1 : 0; ab += SdrRgbDiffers(refA, refB, i) ? 1 : 0;
        }
        Proof(q0 && q1 && q2 && rec1 && rec2 && s1 && s2 && retired && tw == (UINT)W && th == (UINT)H && nA == 0 && nB == 0 && ab * 10 >= n,
              "v  SDR, the target written by %s, %s (DXVK reports it in VkImageLayout %d, format %d): behind the flush the raw image holds the frame's HUD-less picture (%zu of %zu pixels differ),"
              " the next frame then wrote the target (%zu differ from its picture; the two frames' pictures differ on %zu); queries %d %d %d, recorded %d %d, submitted %d %d, retired %d",
              curve != 0.0f ? "the curve's second draw" : "the copy", mode, (int)tLay, (int)tf, nA, n, nB, ab, (int)q0, (int)q1, (int)q2, (int)rec1, (int)rec2, (int)s1, (int)s2, (int)retired);
        Proof(ctl * 10 >= n, "   control: the same compare against the NEXT frame's picture fails on %zu of %zu pixels", ctl, n);
        FgvImageFree(&vk, &X);
        vkDestroyFence_(dev, fence[0], nullptr); vkDestroyFence_(dev, fence[1], nullptr); vkDestroyCommandPool_(dev, pool, nullptr);
        SdrFree(s);
    }
    idev->Release();
}
// --time: THE COST, whole frames as the game orders them (the lines above, at the screen's size; the interface's sprites scaled to it)
static void SdrTime(Dev& v, IDirect3DPixelShader9* cps)
{
    IDirect3DQuery9* q = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_EVENT, &q));
    auto flush = [&]() { q->Issue(D3DISSUE_END); while (q->GetData(nullptr, 0, D3DGETDATA_FLUSH) == S_FALSE) { } };
    LARGE_INTEGER qpf; QueryPerformanceFrequency(&qpf);
    const int N = 300;
    auto run = [&](const char* what, auto&& op, double* least) {
        double us[5];
        for (int r = 0; r < 5; ++r)
        {
            flush(); LARGE_INTEGER a, b; QueryPerformanceCounter(&a);
            for (int i = 0; i < N; ++i) op();
            flush(); QueryPerformanceCounter(&b);
            us[r] = (double)(b.QuadPart - a.QuadPart) * 1e6 / (double)qpf.QuadPart / N;
        }
        std::sort(us, us + 5);
        printf("    %-72s %7.1f  %7.1f\n", what, us[0], us[2]);
        if (least) *least = us[2]; };
    const int sizes[2][2] = { { 3840, 2160 }, { 1920, 1080 } };
    for (const auto& sz : sizes)
    {
        Sdr s; SdrMake(s, v, sz[0], sz[1], D3DFMT_X8R8G8B8, cps, 0x1234567u);
        IDirect3DDevice9* d = v.d;
        auto cp = [&](IDirect3DSurface9* a, IDirect3DSurface9* b, D3DTEXTUREFILTERTYPE f) { d->StretchRect(a, nullptr, b, nullptr, f); };
        auto base0 = [&]() { cp(s.srcS, s.bbS, D3DTEXF_NONE); SdrInterface(s, v, s.bbS); };
        auto sdr0  = [&]() { cp(s.srcS, s.bbS, D3DTEXF_NONE); cp(s.bbS, s.t2S, D3DTEXF_NONE); SdrInterface(s, v, s.bbS); };
        auto base1 = [&]() { cp(s.srcS, s.bbS, D3DTEXF_NONE); SdrInterface(s, v, s.bbS); cp(s.bbS, s.TS, D3DTEXF_POINT); SdrCurveFF(s, s.T, s.bb, 1.0f, 1.0f, nullptr); };
        auto sdr1  = [&]() { cp(s.srcS, s.bbS, D3DTEXF_NONE); cp(s.bbS, s.preS, D3DTEXF_NONE); SdrInterface(s, v, s.bbS); cp(s.bbS, s.TS, D3DTEXF_POINT);
                             SdrCurveFF(s, s.T, s.bb, 1.0f, 1.0f, s.pre); };   // (round 2: the mod's own FgHudSdrReplayD3D)
        base0(); sdr0(); base1(); sdr1(); flush();
        printf("  SDR, THE COST at %d by %d X8R8G8B8, %d frames back to back, the GPU waited for by an event query (five rounds: the least and the median, microseconds a frame)\n", sz[0], sz[1], N);
        double b0 = 0, p0 = 0, b1 = 0, p1 = 0;
        run("one full-screen copy of the back buffer alone (StretchRect)", [&]() { cp(s.bbS, s.t2S, D3DTEXF_NONE); }, nullptr);
        run("FusionFix's curve drawn alone (the second draw's kind)", [&]() { SdrCurve(s, v, s.pre, s.t2, 1.0f, 1.0f); }, nullptr);
        run("a frame, no console gamma: the frame and the interface", base0, &b0);
        run("a frame, no console gamma, WITH THE PRODUCT'S COPY", sdr0, &p0);
        run("a frame with console gamma: the frame, the interface, FusionFix's copy and curve", base1, &b1);
        run("a frame with console gamma, WITH THE PRODUCT'S COPY AND THE CURVE'S SECOND DRAW", sdr1, &p1);
        printf("    THE PRODUCT'S COST A FRAME at %d by %d (medians): no console gamma %.1f us, with console gamma %.1f us\n", sz[0], sz[1], p0 - b0, p1 - b1);
        SdrFree(s);
    }
    q->Release();
}
// w  THE STATE THE SECOND DRAW LEAVES (round 2, the reviewer's third finding): FusionFix's state set and its draw made, with a viewport
//    depth range and a scissor rect of its own; then the mod's FgHudSdrReplayD3D with the same draw; then every state it touches read
//    back against the state before it (render target 0, the viewport, the scissor rect, sampler 0's texture) and some it does not
//    (the pixel shader, constant c7, sampler 0's filter). CONTROL: the second draw made without putting the viewport and the scissor
//    rect back (a bind of a target resets both) must differ.
static void SdrProofW(Dev& v, IDirect3DPixelShader9* cps)
{
    Sdr s; SdrMake(s, v, W, H, D3DFMT_X8R8G8B8, cps, 0x51ED270Bu);
    const D3DVIEWPORT9 vpWant = { 0, 0, (DWORD)W, (DWORD)H, 0.25f, 0.75f };
    const RECT scWant = { 7, 9, 500, 300 };
    gSdrVp = &vpWant; gSdrSc = &scWant;
    struct St { IDirect3DSurface9* rt; D3DVIEWPORT9 vp; RECT sc; IDirect3DBaseTexture9* t0; IDirect3DPixelShader9* ps; float c7[4]; DWORD minf; };
    auto read = [&](St& o) { o = {}; v.d->GetRenderTarget(0, &o.rt); v.d->GetViewport(&o.vp); v.d->GetScissorRect(&o.sc); v.d->GetTexture(0, &o.t0); v.d->GetPixelShader(&o.ps);
                             v.d->GetPixelShaderConstantF(7, o.c7, 1); v.d->GetSamplerState(0, D3DSAMP_MINFILTER, &o.minf);
                             if (o.rt) o.rt->Release(); if (o.t0) o.t0->Release(); if (o.ps) o.ps->Release(); };
    auto same = [](const St& a, const St& b) { return a.rt == b.rt && !memcmp(&a.vp, &b.vp, sizeof(a.vp)) && !memcmp(&a.sc, &b.sc, sizeof(a.sc)) && a.t0 == b.t0 && a.ps == b.ps && !memcmp(a.c7, b.c7, sizeof(a.c7)) && a.minf == b.minf; };
    auto draw = [&]() { return v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kIvtQuad, sizeof(kIvtQuad[0])); };
    IDirect3DSurface9* t2S = nullptr; CK(s.t2->GetSurfaceLevel(0, &t2S));
    SdrCurveFF(s, s.T, s.bb, 1.0f, 1.0f, nullptr);                               // FusionFix's state, and its draw
    St a, b, c; read(a);
    const HRESULT hr = FgHudSdrReplayD3D(v.d, t2S, s.bbS, s.pre, s.T, draw);   // THE MOD'S OWN SECOND DRAW
    read(b);
    IDirect3DSurface9* sbb = nullptr; CK(s.bb->GetSurfaceLevel(0, &sbb)); sbb->Release();
    Proof(SUCCEEDED(hr) && same(a, b) && a.rt == sbb && a.t0 == s.T && a.vp.MinZ == 0.25f && a.sc.left == 7,
          "w  the mod's second draw of the curve (FgHudSdrReplayD3D) leaves render target 0, the viewport (with its depth range 0.25 to 0.75), the scissor rect, sampler 0's texture, the pixel shader, c7 and sampler 0's filter as FusionFix's draw had them");
    // CONTROL: no viewport and no scissor rect put back
    SdrCurveFF(s, s.T, s.bb, 1.0f, 1.0f, nullptr);
    read(a);
    v.d->SetRenderTarget(0, t2S); v.d->SetTexture(0, s.pre); CK(draw()); v.d->SetTexture(0, s.T); v.d->SetRenderTarget(0, s.bbS);
    read(c);
    Proof(!same(a, c) && (c.vp.MinZ != 0.25f || c.sc.left != 7), "   control: a second draw that does not put the viewport and the scissor rect back leaves them changed (depth range %.2f to %.2f, scissor from %ld): the reading sees it",
          c.vp.MinZ, c.vp.MaxZ, (long)c.sc.left);
    gSdrVp = nullptr; gSdrSc = nullptr;
    t2S->Release(); SdrFree(s);
}
static void SdrProofs(Dev& v, bool timeIt, bool layouts)
{
    IDirect3DPixelShader9* cps = nullptr; CK(v.d->CreatePixelShader((const DWORD*)kHdrCgaBlitPS, &cps));
    printf("  SDR (2026-10-20): the HUD-less picture of an 8-bit back buffer, the frame %d by %d, the interface on its rect and a notice in the corner\n", W, H);
    SdrProofU(v, cps);
    SdrProofV(v, cps, layouts ? "DXVK's layouts not unified (--layouts)" : "DXVK's layouts as on this driver");
    SdrProofW(v, cps);
    if (timeIt) SdrTime(v, cps);
    cps->Release();
}

struct HudSt { const char* name; bool cga, g10; float att; bool aw; float curve, dither; };

int main(int argc, char** argv)
{
    const char* dll = nullptr; bool timeIt = false, layouts = false, sdrOnly = false;
    for (int i = 1; i < argc; ++i) { if (!strcmp(argv[i], "--d3d9") && i + 1 < argc) dll = argv[++i]; else if (!strcmp(argv[i], "--time")) timeIt = true; else if (!strcmp(argv[i], "--layouts")) layouts = true; else if (!strcmp(argv[i], "--sdr")) sdrOnly = true; }
    SetEnvironmentVariableA("DXVK_LOG_LEVEL", "none");
    // --layouts: DXVK keeps each image in a layout of its kind and moves it with barriers of its own, as on a driver without
    // VK_KHR_unified_image_layouts (tools\test-fg-vklines runs its rows both ways for the same reason)
    if (layouts) SetEnvironmentVariableA("DXVK_CONFIG", "dxvk.enableUnifiedImageLayouts = False");
    HMODULE m = dll ? LoadLibraryA(dll) : LoadLibraryA("d3d9.dll");
    if (!m) { printf("  FAIL  could not load %s\n", dll ? dll : "d3d9.dll"); return 2; }
    typedef IDirect3D9* (WINAPI* PFN)(UINT); PFN create = (PFN)GetProcAddress(m, "Direct3DCreate9");
    char path[MAX_PATH]; GetModuleFileNameA(m, path, MAX_PATH);
    printf("The HUD-less picture's picture proof: the encodes of source/fg/shaders over the real encode's own output, through %s\n", path);
    IDirect3D9* d3d = create ? create(D3D_SDK_VERSION) : nullptr;
    if (!d3d) { printf("  FAIL  Direct3DCreate9\n"); return 2; }
    D3DADAPTER_IDENTIFIER9 id = {}; d3d->GetAdapterIdentifier(0, 0, &id); printf("  adapter: %s\n", id.Description);
    WNDCLASSA wc = {}; wc.lpfnWndProc = DefWindowProcA; wc.hInstance = GetModuleHandleA(nullptr); wc.lpszClassName = "hudlessproof"; RegisterClassA(&wc);
    HWND hw = CreateWindowA("hudlessproof", "hudless", WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr, wc.hInstance, nullptr);   // never shown
    Dev v; Setup(v, d3d, hw, W, H);
    if (sdrOnly) { SdrProofs(v, timeIt, layouts); printf("%s\n", gFails ? "FAIL" : "PASS"); return gFails ? 1 : 0; }
    IDirect3DPixelShader9 *enc = nullptr, *encC = nullptr, *g10 = nullptr, *g10C = nullptr;
    CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodePS, &enc));     CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeCgaPS, &encC));
    CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeG10PS, &g10));  CK(v.d->CreatePixelShader((const DWORD*)kHdrEncodeCgaG10PS, &g10C));
    IDirect3DPixelShader9* mrt[4] = {};
    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtPS, &mrt[0]));    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtCgaPS, &mrt[1]));
    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtG10PS, &mrt[2])); CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtCgaG10PS, &mrt[3]));
    IDirect3DTexture9 *rt32b = nullptr, *rt16b = nullptr;              // the second targets
    CK(v.d->CreateTexture(W, H, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A32B32G32R32F, D3DPOOL_DEFAULT, &rt32b, nullptr));
    CK(v.d->CreateTexture(W, H, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &rt16b, nullptr));
    auto realOf = [&](bool cga, bool sharp) { return cga ? (sharp ? g10C : encC) : (sharp ? g10 : enc); };
    auto mrtOf  = [&](bool cga, bool sharp) { return mrt[(cga ? 1 : 0) | (sharp ? 2 : 0)]; };
    auto every = [](int, int) { return true; };
    auto outside = [](int x, int y) { return !InHud(x, y); };
    auto onHud = [](int x, int y) { return InHud(x, y); };
    // (console gamma NONE in the plain states: the mask puts FusionFix's curve on the copy before it compares, because in the
    // game FusionFix has drawn it into the frame by then, and this harness's frame carries no curve: g10.cpp's own note)
    const HudSt hs[6] = {
        { "plain, no sharpening",                              false, false, 1.0f, true,  0.0f, 0.0f },
        { "plain, sharpening 1, above white shown",            false, true,  1.0f, true,  0.0f, 0.0f },
        { "plain, sharpening 0.5, ceiling 1",                  false, true,  0.5f, false, 0.0f, 0.0f },
        { "absorbed (the Xbox curve, dither on), no sharpening", true, false, 1.0f, true,  1.0f, 1.0f },
        { "absorbed (the Xbox curve, dither on), sharpening 1",  true, true,  1.0f, true,  1.0f, 1.0f },
        { "absorbed (the PS3 curve, dither off), sharpening 0.5", true, true, 0.5f, true,  2.0f, 0.0f } };
    IDirect3DTexture9* txR[12]; memcpy(txR, v.tex, sizeof(txR)); txR[0] = v.tex[11];   // the frame before the interface, bound as the frame
    size_t nHud = 0; for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) nHud += InHud(x, y) ? 1 : 0;
    printf("  the frame: %d by %d, the interface on %zu pixels (they differ from the copy before it by 70 percent)\n", W, H, nHud);
    for (const HudSt& h : hs) for (const float level : { 1.0f, 0.15f })
    {
        State s; Base(s, h.name, h.cga, h.att, h.aw);
        if (!h.cga) s.c[8][0] = 0.0f; else { s.c[7][0] = h.curve; s.c[7][1] = h.dither; s.c[8][0] = h.curve; }
        float c[9][4]; memcpy(c, s.c, sizeof(c)); if (h.g10) G10Const(c, s); c[1][1] = level;
        IDirect3DPixelShader9* const real = realOf(h.cga, h.g10);
        const Out P = Render2(v, real, c, v.tex);
        const Out R = Render2(v, real, c, txR);
        printf("  %s; the real encode's UI level %.2f\n", h.name, level);
        const int64_t lim = h.cga ? 1024 : 64;                          // (the two shaders' compile noise: 64 float32 steps plain, 1024 absorbed)
        float c2[9][4]; memcpy(c2, c, sizeof(c2)); c2[0][0] = 100.0f;      // (paper white: the encode's last multiply, for the controls)
        // ---- THE SECOND-TARGET FORM (HDR_FGMRT): one draw, two targets
        const Out2 M = RenderMrt(v, mrtOf(h.cga, h.g10), c, v.tex, rt32b, rt16b);
        // c  THE PRESENTED PICTURE IS THE REAL ENCODE'S: target 0 against the shipping encode, every pixel. One text compiled twice:
        //    held to the compile noise (64 float32 steps plain, 1024 absorbed, single FP16 steps), and said.
        const Cmp mc = Compare(v, P, M.c0, every);
        Proof(mc.maxUlp <= lim && mc.nanMismatch == 0 && mc.maxHalfStep <= 1,
              "c  the second-target form: its presented picture (target 0) is the real encode's on every pixel (the limit %lld steps): %s", (long long)lim, Tell(mc));
        // d  OUTSIDE THE INTERFACE THE HUD-LESS PICTURE (target 1) IS ITS OWN PRESENTED PICTURE (target 0) BIT FOR BIT
        const Cmp md = Compare(v, M.c0, M.c1, outside);
        Proof(md.diff32 == 0 && md.diff16 == 0 && md.nanMismatch == 0, "d  the second-target form: outside the interface the HUD-less picture is the presented picture bit for bit: %s", Tell(md));
        // e  INSIDE THE INTERFACE IT IS THE REAL ENCODE OF THE FRAME BEFORE THE INTERFACE, on 99 percent or more of its pixels
        std::vector<uint8_t> actM((size_t)W * H, 0); size_t nActM = 0;
        for (int y = 0; y < H; ++y) for (int x = 0; x < W; ++x) if (InHud(x, y)) { const size_t i = (size_t)y * W + x; if (PixelDiffers(M.c0, M.c1, i)) { actM[i] = 1; nActM++; } }
        auto wroteM = [&](int x, int y) { return InHud(x, y) && actM[(size_t)y * W + x]; };
        const Cmp me = Compare(v, R, M.c1, wroteM);
        Proof(nActM * 100 >= nHud * 99 && me.maxUlp <= lim && me.nanMismatch == 0 && me.maxHalfStep <= 1,
              "e  the second-target form: inside the interface %zu of %zu pixels differ from the presented picture, and there the HUD-less picture is the real encode of the frame before the interface (the limit %lld steps): %s",
              nActM, nHud, (long long)lim, Tell(me));
        // the controls
        const Cmp mca = Compare(v, M.c0, M.c1, onHud);
        const Out2 M2 = RenderMrt(v, mrtOf(h.cga, h.g10), c2, v.tex, rt32b, rt16b);
        const Cmp mcd = Compare(v, M2.c0, M.c1, outside);              // d's reader against a presented picture of another paper white
        const Cmp mce = Compare(v, Render2(v, real, c2, txR), M.c1, wroteM);
        Proof(mca.diff32 * 100 >= nHud * 99 && mcd.diff32 * 2 > mcd.n && (mce.maxUlp > lim || mce.nanMismatch),
              "   controls: \"bit for bit\" FAILS on the interface's own pixels (%zu of %zu differ) and against a presented picture of another paper white (%zu of %zu differ); \"the real encode of the frame before the interface\" FAILS against another paper white (%lld steps)",
              mca.diff32, mca.n, mcd.diff32, mcd.n, (long long)mce.maxUlp);
    }
    // THE SECOND-TARGET FORM WITH THE DEVICE'S OWN BACK BUFFER AS TARGET 0, as the game binds it: both targets are written by the
    // one draw. The back buffer here is 64 by 64 and 8 bits; the second target is made like it. The frame's top-left
    // corner has no interface in it, so the two must hold the same bytes, and not the colour they were cleared to.
    // CONTROL: the same draw with no second target bound leaves the second target at its clear colour.
    {
        IDirect3DSurface9 *bb = nullptr, *t1 = nullptr, *sysA = nullptr, *sysB = nullptr; IDirect3DTexture9* tt = nullptr;
        CK(v.d->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb));
        D3DSURFACE_DESC bd; CK(bb->GetDesc(&bd));
        CK(v.d->CreateTexture(bd.Width, bd.Height, 1, D3DUSAGE_RENDERTARGET, bd.Format, D3DPOOL_DEFAULT, &tt, nullptr)); CK(tt->GetSurfaceLevel(0, &t1));
        CK(v.d->CreateOffscreenPlainSurface(bd.Width, bd.Height, bd.Format, D3DPOOL_SYSTEMMEM, &sysA, nullptr));
        CK(v.d->CreateOffscreenPlainSurface(bd.Width, bd.Height, bd.Format, D3DPOOL_SYSTEMMEM, &sysB, nullptr));
        State s; Base(s, "bb", false, 1.0f, true); s.c[8][0] = 0.0f;
        float c[9][4]; memcpy(c, s.c, sizeof(c)); c[1][1] = 1.0f;
        auto read = [&](IDirect3DSurface9* from, IDirect3DSurface9* sys) {
            CK(v.d->GetRenderTargetData(from, sys)); std::vector<BYTE> o((size_t)bd.Width * bd.Height * 4); D3DLOCKED_RECT lr; CK(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY));
            for (UINT y = 0; y < bd.Height; ++y) memcpy(&o[(size_t)y * bd.Width * 4], (BYTE*)lr.pBits + (size_t)y * lr.Pitch, (size_t)bd.Width * 4);
            sys->UnlockRect(); return o; };
        auto drawBb = [&](bool second) {
            CK(v.d->ColorFill(bb, nullptr, D3DCOLOR_ARGB(255, 255, 0, 255))); CK(v.d->ColorFill(t1, nullptr, D3DCOLOR_ARGB(255, 0, 255, 0)));
            if (second) CK(v.d->SetRenderTarget(1, t1));
            CK(v.d->SetRenderTarget(0, bb));
            const D3DVIEWPORT9 vp = { 0, 0, bd.Width, bd.Height, 0.0f, 1.0f }; v.d->SetViewport(&vp);
            v.d->SetRenderState(D3DRS_ZENABLE, FALSE); v.d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE); v.d->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            v.d->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE); v.d->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF); v.d->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
            for (int i = 0; i < 12; ++i) v.d->SetTexture(i, v.tex[i]);
            v.d->SetVertexShader(v.vs); v.d->SetVertexDeclaration(v.decl); v.d->SetPixelShader(mrt[0]);
            v.d->SetPixelShaderConstantF(0, &c[0][0], 9);
            CK(v.d->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, kIvtQuad, sizeof(kIvtQuad[0])));
            CK(v.d->SetRenderTarget(1, nullptr)); };
        drawBb(true);
        const std::vector<BYTE> a0 = read(bb, sysA), a1 = read(t1, sysB);
        size_t same = 0, cleared0 = 0, cleared1 = 0; const size_t n = (size_t)bd.Width * bd.Height;
        for (size_t i = 0; i < n; ++i)
        {
            if (!memcmp(&a0[4 * i], &a1[4 * i], 3)) same++;
            if (a0[4 * i] == 255 && a0[4 * i + 1] == 0 && a0[4 * i + 2] == 255) cleared0++;
            if (a1[4 * i] == 0 && a1[4 * i + 1] == 255 && a1[4 * i + 2] == 0) cleared1++;
        }
        Proof(same == n && cleared0 * 10 < n && cleared1 * 10 < n,
              "g  the second-target form with the device's own back buffer (%u by %u, format %d) as target 0: both targets written by the one draw: %zu of %zu pixels the same in both, %zu and %zu still at their clear colour",
              bd.Width, bd.Height, (int)bd.Format, same, n, cleared0, cleared1);
        drawBb(false);
        const std::vector<BYTE> b1 = read(t1, sysB);
        size_t stay = 0; for (size_t i = 0; i < n; ++i) if (b1[4 * i] == 0 && b1[4 * i + 1] == 255 && b1[4 * i + 2] == 0) stay++;
        Proof(stay == n, "   control: with no second target bound the second target keeps its clear colour (%zu of %zu pixels)", stay, n);
        sysA->Release(); sysB->Release(); t1->Release(); tt->Release(); bb->Release();
    }
    RawProof(v, mrt[0], rt16b, layouts ? "DXVK's layouts not unified (--layouts)" : "DXVK's layouts as on this driver", v.tex);
    IDirect3DPixelShader9* mrtOv[4] = {};                              // (2026-10-12) the four of a frame with a 3D overlay ahead of the interface, HDR_FGOV
    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtOvPS, &mrtOv[0]));    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtOvCgaPS, &mrtOv[1]));
    CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtOvG10PS, &mrtOv[2])); CK(v.d->CreatePixelShader((const DWORD*)kHdrFgMrtOvCgaG10PS, &mrtOv[3]));
    PhoneProof(v, enc, mrt[0], mrtOv[0], rt32b, rt16b, layouts ? "DXVK's layouts not unified (--layouts)" : "DXVK's layouts as on this driver");
    if (timeIt)
    {
        const int TW = 3840, TH = 2160, N = 300;
        printf("  THE COST at %d by %d FP16, %d draws back to back, the GPU waited for by an event query (five rounds: the least and the median, microseconds a draw; the CPU's side of a draw is in it)\n", TW, TH, N);
        std::vector<Px> frame((size_t)TW * TH); for (auto& p : frame) p = { Rnd(), Rnd(), Rnd() };
        std::vector<Px> pre = frame; size_t hudPx = 0;
        for (int y = 1620; y < TH; ++y) for (int x = 0; x < 960; ++x) { Px& p = pre[(size_t)y * TW + x]; p = { p.r * 0.3f, p.g * 0.3f, p.b * 0.3f }; hudPx++; }
        IDirect3DTexture9 *t0 = MakeHalfTex(v.d, TW, TH, frame), *t11 = MakeHalfTex(v.d, TW, TH, pre), *rt = nullptr, *src = nullptr;
        CK(v.d->CreateTexture(TW, TH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &rt, nullptr));
        CK(v.d->CreateTexture(TW, TH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &src, nullptr));
        IDirect3DSurface9 *rtS = nullptr, *srcS = nullptr; CK(rt->GetSurfaceLevel(0, &rtS)); CK(src->GetSurfaceLevel(0, &srcS));
        IDirect3DQuery9* q = nullptr; CK(v.d->CreateQuery(D3DQUERYTYPE_EVENT, &q));
        Dev t = v; t.w = TW; t.h = TH;
        IDirect3DTexture9* tx[12]; memcpy(tx, v.tex, sizeof(tx)); tx[0] = t0; tx[11] = t11;
        auto consts = [&](float (&c)[9][4], bool cga, bool sharp) {
            State s; Base(s, "t", cga, 1.0f, true);
            if (!cga) s.c[8][0] = 0.0f;
            s.c[1][2] = 1.0f / TW; s.c[1][3] = 1.0f / TH;
            memcpy(c, s.c, sizeof(s.c)); if (sharp) G10Const(c, s); c[1][1] = 1.0f; };
        auto flush = [&]() { q->Issue(D3DISSUE_END); while (q->GetData(nullptr, 0, D3DGETDATA_FLUSH) == S_FALSE) { } };
        LARGE_INTEGER qpf; QueryPerformanceFrequency(&qpf);
        auto run = [&](const char* what, auto&& op) {
            double us[5];
            for (int r = 0; r < 5; ++r)
            {
                flush(); LARGE_INTEGER a, b; QueryPerformanceCounter(&a);
                for (int i = 0; i < N; ++i) op();
                flush(); QueryPerformanceCounter(&b);
                us[r] = (double)(b.QuadPart - a.QuadPart) * 1e6 / (double)qpf.QuadPart / N;
            }
            std::sort(us, us + 5);
            printf("    %-64s %7.1f  %7.1f\n", what, us[0], us[2]); };
        float cP[9][4], cS[9][4], cCS[9][4]; consts(cP, false, false); consts(cS, false, true); consts(cCS, true, true);
        Draw(t, enc, cP, src, tx); Draw(t, enc, cP, rt, tx); flush();
        printf("    the interface on %zu of %zu pixels (%.2f percent)\n", hudPx, (size_t)TW * TH, 100.0 * (double)hudPx / ((double)TW * TH));
        run("one full-screen copy (StretchRect, the calibration)", [&]() { v.d->StretchRect(srcS, nullptr, rtS, nullptr, D3DTEXF_NONE); });
        run("the real encode, plain", [&]() { Draw(t, enc, cP, rt, tx); });
        run("the real encode, sharpening", [&]() { Draw(t, g10, cS, rt, tx); });
        run("the real encode, absorbed and sharpening", [&]() { Draw(t, g10C, cCS, rt, tx); });
        // THE SECOND-TARGET FORM (2026-10-11): the real encode and the HUD-less picture in ONE draw into two targets. What it costs
        // more than the real encode alone is the whole of the HUD-less picture's draw in that form.
        IDirect3DTexture9* rt2 = nullptr; CK(v.d->CreateTexture(TW, TH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &rt2, nullptr));
        float cC[9][4]; consts(cC, true, false);
        // (an interface on 2 percent of the screen, what the game's own check read on 2026-10-11: 1.6 to 2.2; and none at all)
        std::vector<Px> pre2 = frame; size_t hud2 = 0;
        for (int y = 1800; y < TH; ++y) for (int x = 0; x < 480; ++x) { Px& p = pre2[(size_t)y * TW + x]; p = { p.r * 0.3f, p.g * 0.3f, p.b * 0.3f }; hud2++; }
        IDirect3DTexture9* t11b = MakeHalfTex(v.d, TW, TH, pre2);
        IDirect3DTexture9* tx2[12]; memcpy(tx2, tx, sizeof(tx2)); tx2[11] = t11b;
        IDirect3DTexture9* tx0[12]; memcpy(tx0, tx, sizeof(tx0)); tx0[11] = t0;
        DrawMrt(t, mrt[0], cP, rt, rt2, tx); flush();
        printf("    THE SECOND-TARGET FORM; the interface on %.2f percent, then on %.2f percent (%zu pixels), then on none\n", 100.0 * (double)hudPx / ((double)TW * TH), 100.0 * (double)hud2 / ((double)TW * TH), hud2);
        run("the real encode, plain (again, beside the lines below)", [&]() { Draw(t, enc, cP, rt, tx); });
        run("the second-target encode, plain", [&]() { DrawMrt(t, mrt[0], cP, rt, rt2, tx); });
        run("the second-target encode, plain, the interface on 2 percent", [&]() { DrawMrt(t, mrt[0], cP, rt, rt2, tx2); });
        run("the second-target encode, plain, no interface", [&]() { DrawMrt(t, mrt[0], cP, rt, rt2, tx0); });
        run("the real encode, plain, the interface on 2 percent", [&]() { Draw(t, enc, cP, rt, tx2); });
        run("the real encode, absorbed", [&]() { Draw(t, encC, cC, rt, tx); });
        run("the second-target encode, absorbed", [&]() { DrawMrt(t, mrt[1], cC, rt, rt2, tx); });
        run("the real encode, sharpening (again)", [&]() { Draw(t, g10, cS, rt, tx); });
        run("the second-target encode, sharpening", [&]() { DrawMrt(t, mrt[2], cS, rt, rt2, tx); });
        run("the real encode, absorbed and sharpening (again)", [&]() { Draw(t, g10C, cCS, rt, tx); });
        run("the second-target encode, absorbed and sharpening", [&]() { DrawMrt(t, mrt[3], cCS, rt, rt2, tx); });
        run("one full-screen copy (StretchRect), again", [&]() { v.d->StretchRect(srcS, nullptr, rtS, nullptr, D3DTEXF_NONE); });
        // WHOLE FRAMES, AS THE GAME ORDERS THEM (2026-10-11). The lines above draw one thing 300 times over the same pixels, which
        // is not how a frame runs (the second-target encode reads CHEAPER than the real encode alone there: the same pixels
        // shaded 300 times inside one render pass wait for each other). So here each of the 300 rounds is ONE FRAME'S WORK
        // ON THE GAME'S SIDE, every copy a full-screen copy between FP16 targets (D3D9's own, standing for the worker's raw
        // ones), and a form's cost is its frame less the frame of the two-image set:
        //   the frame written into the "back buffer"; the encode's copy out of it; the encode; then by form
        //   TWO IMAGES        back buffer into PRESENT, into HOLD
        //   THE SECOND TARGET the encode writes both; back buffer into PRESENT, second target into HUDLESS, back buffer into HOLD
        //   NO INTERFACE      before: two images and the back buffer into HUDLESS once more; with PRESENT bound twice: two images
        {
            IDirect3DTexture9 *pris = nullptr, *tPr = nullptr, *tHo = nullptr, *tHu = nullptr;
            IDirect3DSurface9 *prisS = nullptr, *prS = nullptr, *hoS = nullptr, *huS = nullptr, *rt2S = nullptr, *sys = nullptr;
            for (IDirect3DTexture9** tp : { &pris, &tPr, &tHo, &tHu }) CK(v.d->CreateTexture(TW, TH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, tp, nullptr));
            CK(pris->GetSurfaceLevel(0, &prisS)); CK(tPr->GetSurfaceLevel(0, &prS)); CK(tHo->GetSurfaceLevel(0, &hoS)); CK(tHu->GetSurfaceLevel(0, &huS)); CK(rt2->GetSurfaceLevel(0, &rt2S));
            // the frame itself, as the game's back buffer holds it before the encode (the same values as t0, so the mask is the interface alone)
            CK(v.d->CreateOffscreenPlainSurface(TW, TH, D3DFMT_A16B16G16R16F, D3DPOOL_SYSTEMMEM, &sys, nullptr));
            { D3DLOCKED_RECT lr; CK(sys->LockRect(&lr, nullptr, 0));
              for (int y = 0; y < TH; ++y) { uint16_t* row = (uint16_t*)((BYTE*)lr.pBits + (size_t)y * lr.Pitch);
                  for (int x = 0; x < TW; ++x) { const Px& p = frame[(size_t)y * TW + x]; row[4 * x] = ToHalf(p.r); row[4 * x + 1] = ToHalf(p.g); row[4 * x + 2] = ToHalf(p.b); row[4 * x + 3] = ToHalf(1.0f); } }
              sys->UnlockRect(); }
            CK(v.d->UpdateSurface(sys, nullptr, prisS, nullptr));
            auto cp = [&](IDirect3DSurface9* a, IDirect3DSurface9* b) { v.d->StretchRect(a, nullptr, b, nullptr, D3DTEXF_NONE); };
            IDirect3DTexture9* fx[12]; memcpy(fx, tx, sizeof(fx)); fx[0] = src;      // the encode samples its copy of the back buffer
            IDirect3DTexture9* fx2[12]; memcpy(fx2, fx, sizeof(fx2)); fx2[11] = t11b; // the interface on 2 percent
            IDirect3DTexture9* fx0[12]; memcpy(fx0, fx, sizeof(fx0)); fx0[11] = t0;   // no interface
            struct Form { const char* name; IDirect3DPixelShader9 *real, *mrt; float (*c)[4]; IDirect3DTexture9** t; };
            auto head = [&]() { cp(prisS, rtS); cp(rtS, srcS); };
            auto frameTwo = [&](const Form& f) { head(); Draw(t, f.real, *(float(*)[9][4])f.c, rt, *(IDirect3DTexture9*(*)[12])f.t); cp(rtS, prS); cp(rtS, hoS); };
            auto frameMrt = [&](const Form& f) { head(); DrawMrt(t, f.mrt, *(float(*)[9][4])f.c, rt, rt2, *(IDirect3DTexture9*(*)[12])f.t); cp(rtS, prS); cp(rt2S, huS); cp(rtS, hoS); };
            auto frameSame = [&](const Form& f) { head(); Draw(t, f.real, *(float(*)[9][4])f.c, rt, *(IDirect3DTexture9*(*)[12])f.t); cp(rtS, prS); cp(rtS, huS); cp(rtS, hoS); };
            auto frameHead = [&](const Form& f) { head(); Draw(t, f.real, *(float(*)[9][4])f.c, rt, *(IDirect3DTexture9*(*)[12])f.t); };
            auto frameMrtHead = [&](const Form& f) { head(); DrawMrt(t, f.mrt, *(float(*)[9][4])f.c, rt, rt2, *(IDirect3DTexture9*(*)[12])f.t); };
            const Form forms[5] = {
                { "plain, the interface on 6.25 percent", enc, mrt[0], cP, fx },
                { "plain, the interface on 2 percent", enc, mrt[0], cP, fx2 },
                { "absorbed, the interface on 2 percent", encC, mrt[1], cC, fx2 },
                { "sharpening, the interface on 2 percent", g10, mrt[2], cS, fx2 },
                { "absorbed and sharpening, the interface on 2 percent", g10C, mrt[3], cCS, fx2 } };
            frameMrt(forms[0]); flush();
            printf("    WHOLE FRAMES, the game's side of one pair (300 frames back to back, five rounds: the least and the median, microseconds a frame)\n");
            for (const Form& f : forms)
            {
                char nm[128];
                _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s: the frame to its encode", f.name);                    run(nm, [&]() { frameHead(f); });
                _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s: to the second-target encode", f.name);                run(nm, [&]() { frameMrtHead(f); });
                _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s: TWO IMAGES", f.name);                                 run(nm, [&]() { frameTwo(f); });
                _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s: THE SECOND TARGET", f.name);                          run(nm, [&]() { frameMrt(f); });
            }
            const Form none = { "plain, no interface", enc, mrt[0], cP, fx0 };
            run("plain, no interface: TWO IMAGES (and PRESENT bound twice)", [&]() { frameTwo(none); });
            run("plain, no interface: the back buffer into HUDLESS too (before)", [&]() { frameSame(none); });
            // THE PHONE UP (2026-10-12, two copy points and the exact cover; fg_hud.h, "TWO COPY POINTS"). A frame with a phone on 6.5 percent
            // of the screen inside a viewport of 1536 by 1080, and the interface on 2 percent. AS IT WAS (the first form): the
            // encode every frame draws with, its one copy taken ahead of the phone: the frame's work is THE SECOND TARGET's above
            // with another texture on the copy's sampler. NOW: ONE COPY MORE of the back buffer (the overlay's own, ahead of the
            // phone) and the overlay's encode, which reads both copies and the rect. The frame's cost is the difference.
            {
                std::vector<Px> preOv = pre2;                      // the frame ahead of the phone: the scene behind it where the phone stands
                for (int y = 1100; y < 2000; ++y) for (int x = 2800; x < 3400; ++x) { Px& p = preOv[(size_t)y * TW + x]; p = { p.r * 0.5f, p.g * 0.5f, p.b * 0.5f }; }
                IDirect3DTexture9 *t5 = MakeHalfTex(v.d, TW, TH, preOv), *tOv = nullptr; IDirect3DSurface9* ovS = nullptr;
                CK(v.d->CreateTexture(TW, TH, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A16B16G16R16F, D3DPOOL_DEFAULT, &tOv, nullptr)); CK(tOv->GetSurfaceLevel(0, &ovS));
                IDirect3DTexture9* fxWas[12]; memcpy(fxWas, fx2, sizeof(fxWas)); fxWas[11] = t5;                    // the first form: the copy ahead of the phone as the encode's own
                IDirect3DTexture9* fxOv[12]; memcpy(fxOv, fx2, sizeof(fxOv)); fxOv[5] = t5;                        // both copies
                OvRect(t, 2304, 1080, 3840, 2160);
                struct PhForm { const char* name; IDirect3DPixelShader9 *mrt, *ov; float (*c)[4]; };
                const PhForm pf[2] = { { "plain", mrt[0], mrtOv[0], cP }, { "absorbed and sharpening", mrt[3], mrtOv[3], cCS } };
                for (const PhForm& f : pf)
                {
                    auto frameWas = [&]() { head(); DrawMrt(t, f.mrt, *(float(*)[9][4])f.c, rt, rt2, fxWas); cp(rtS, prS); cp(rt2S, huS); cp(rtS, hoS); };
                    auto frameNow = [&]() { cp(prisS, rtS); cp(rtS, ovS); cp(rtS, srcS); DrawMrt(t, f.ov, *(float(*)[9][4])f.c, rt, rt2, fxOv); cp(rtS, prS); cp(rt2S, huS); cp(rtS, hoS); };
                    frameWas(); frameNow(); flush();
                    char nm[160];
                    _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s, THE PHONE UP: the second-target encode alone, as it was", f.name);        run(nm, [&]() { DrawMrt(t, f.mrt, *(float(*)[9][4])f.c, rt, rt2, fxWas); });
                    _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s, THE PHONE UP: the overlay's encode alone (both copies)", f.name);         run(nm, [&]() { DrawMrt(t, f.ov, *(float(*)[9][4])f.c, rt, rt2, fxOv); });
                    _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s, THE PHONE UP: THE SECOND TARGET, as it was (one copy)", f.name);          run(nm, frameWas);
                    _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s, THE PHONE UP: THE SECOND TARGET, two copy points", f.name);               run(nm, frameNow);
                    _snprintf_s(nm, sizeof(nm), _TRUNCATE, "%s, no phone: THE SECOND TARGET (again, beside them)", f.name);               run(nm, [&]() { head(); DrawMrt(t, f.mrt, *(float(*)[9][4])f.c, rt, rt2, fx2); cp(rtS, prS); cp(rt2S, huS); cp(rtS, hoS); });
                }
                ovS->Release(); tOv->Release(); t5->Release();
            }
            sys->Release(); rt2S->Release(); huS->Release(); hoS->Release(); prS->Release(); prisS->Release(); tHu->Release(); tHo->Release(); tPr->Release(); pris->Release();
        }
        rt2->Release(); t11b->Release();
        q->Release(); rtS->Release(); srcS->Release(); rt->Release(); src->Release(); t0->Release(); t11->Release();
    }
    SdrProofs(v, timeIt, layouts);                                  // (SDR, 2026-10-20: proofs u and v, and with --time their cost)
    printf("%s\n", gFails ? "FAIL" : "PASS");
    return gFails ? 1 : 0;
}
