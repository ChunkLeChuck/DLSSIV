// IVMotionVectors.fx - real geometric motion vectors for GTA IV.
//
// Writes texMotionVectors as DELTA-UV satisfying prev_uv = uv + mv, which is
// the contract DLSS5-Feeder's provider 0 expects. Do NOT scale to pixels here;
// the Feeder does that conversion itself.
//
// Every other motion-vector provider in that list (iMMERSE Launchpad, VORT,
// LumeniteFX) estimates motion by matching pixels between two finished frames.
// This one does not estimate anything: it reads the engine's own camera
// matrices, published by IVMotionVectors.asi and carried in by
// IVMotionVectors.addon32, and reprojects the depth buffer through them. For
// static geometry the result is arithmetic, not a guess.
//
// TECHNIQUE ORDER MATTERS: this must run BEFORE DLSS5_Feed.fx.
//
// KNOWN LIMITATION, stated plainly: this is CAMERA motion only. Moving
// vehicles and pedestrians get the camera's vector, which is wrong for them.
// Optical-flow providers do better on traffic and much worse on everything
// else. Per-object vectors are the next milestone.

#include "ReShade.fxh"

// ---------------------------------------------------------------------------
// Set every frame by IVMotionVectors.addon32. Zero when it is not running,
// which drives ivmv_misc.z (valid) to 0 and makes this shader output nothing.
//
// Passed as four float4 ROWS rather than a float4x4 on purpose: HLSL's default
// matrix packing is column-major, ours is row-major, and doing the multiply by
// hand here removes a whole class of silent transpose bugs.
// ---------------------------------------------------------------------------

uniform float4 ivmv_reproj0 < ui_type = "drag"; ui_label = "(set by addon)"; >  = float4(1,0,0,0);
uniform float4 ivmv_reproj1 < ui_type = "drag"; ui_label = "(set by addon)"; >  = float4(0,1,0,0);
uniform float4 ivmv_reproj2 < ui_type = "drag"; ui_label = "(set by addon)"; >  = float4(0,0,1,0);
uniform float4 ivmv_reproj3 < ui_type = "drag"; ui_label = "(set by addon)"; >  = float4(0,0,0,1);

// (a, b, c, d_p) = (P[0][0], P[1][1], P[2][2], P[3][2]) of the current frame.
uniform float4 ivmv_proj     < ui_type = "drag"; ui_label = "(set by addon)"; > = float4(0,0,0,0);
uniform float4 ivmv_projPrev < ui_type = "drag"; ui_label = "(set by addon)"; > = float4(0,0,0,0);

// (nearClip, farClip, valid, logDepthObserved)
uniform float4 ivmv_misc     < ui_type = "drag"; ui_label = "(set by addon)"; > = float4(0,0,0,0);
uniform float  ivmv_frame    < ui_type = "drag"; ui_label = "(set by addon)"; > = 0.0;

// ---------------------------------------------------------------------------
// User controls
// ---------------------------------------------------------------------------

uniform int ivmv_debug <
    ui_type  = "combo";
    ui_label = "Debug view";
    ui_items = "Off (write motion vectors)\0"
               "Raw depth\0"
               "Decoded Z (linear, 0-200 units)\0"
               "Motion vectors (colour)\0"
               "Reprojection test\0"
               "Diagnostics\0"
               "Engine velocity target (chain test)\0"
               "Object velocity coverage\0"
               "Object velocity depth agreement\0"
               "Motion vector ERROR (disocclusion masked)\0";
    ui_tooltip = "Motion vector ERROR is the one that answers whether the vectors "
                 "are right: BLACK is correct, red is a vector pointing at the wrong "
                 "surface. Grey is disoccluded and dark blue has no history, both "
                 "excluded rather than counted. The HUD always lights up because it "
                 "has no motion vector at all. "
                 "Reprojection test samples the PREVIOUS frame at uv+mv. With a "
                 "static scene it should look identical to the current frame. "
                 "That is the strongest single check of the whole chain.";
> = 0;

uniform int ivmv_depth_mode <
    ui_type  = "combo";
    ui_label = "Depth encoding";
    ui_items = "Auto (use what the .asi observed)\0"
               "Force logarithmic (FusionShaders)\0"
               "Force standard\0";
    ui_tooltip = "GTA IV with FusionShaders installed writes LOGARITHMIC depth: "
                 "d = log2(Z/near)/log2(far/near). Note this is NOT the same as "
                 "ReShade's own RESHADE_DEPTH_INPUT_IS_LOGARITHMIC, which uses "
                 "the Outerra formula with C=0.01. Leave that ReShade setting OFF.";
> = 0;

uniform float ivmv_err_threshold <
    ui_type = "slider"; ui_label = "Debug: error threshold";
    ui_min = 0.002; ui_max = 0.100; ui_step = 0.002;
    ui_tooltip = "How far outside the reprojected neighbourhood a pixel has to "
                 "fall before the meter counts it as wrong. 0.02 is about 2% of "
                 "full scale and is a strict standard.";
> = 0.020;

uniform float ivmv_debug_scale <
    ui_type = "slider"; ui_label = "Debug: motion vector gain";
    ui_min = 1.0; ui_max = 200.0;
> = 40.0;

// ---------------------------------------------------------------------------
// Resources
// ---------------------------------------------------------------------------

// DECLARED BYTE-IDENTICALLY TO DLSS5_Feed.fx line 123, annotation included:
//     texture texMotionVectors < pooled = false; > { ... Format = RG16F; };
// ReShade shares a texture across effects by name, and the declarations have to
// agree. Ours omitted `pooled = false` and theirs has it, which is the one
// difference that could have left us writing a DIFFERENT texture from the one
// they sample -- with no error from either side, just silently zero vectors.
// Match it exactly rather than reason about whether it matters.
texture texMotionVectors < pooled = false; > { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RG16F; };
sampler sMotionVectors   { Texture = texMotionVectors; };

// Previous frame's colour, for the reprojection test only.
//
// TWO textures, not one, and the reason matters. The first version saved the
// backbuffer AFTER the display pass had already written the reprojected image
// into it, so "previous frame" was actually our own output. Each frame then
// reprojected a reprojection and the smear compounded -- which reads exactly
// like motion vectors that undershoot, and is not. Accumulation is the
// signature of feedback, not of a scale error.
//
// So: snapshot the PRISTINE backbuffer before the display pass, and roll it
// into the history afterwards.
// The engine-side per-object velocity target, rendered by IVMotionVectors.asi
// and bound here by IVMotionVectors.addon32 via update_texture_bindings. No
// size or format: a semantic-bound texture takes them from the resource the
// addon supplies, exactly like ReShade's own DEPTH semantic.
texture texIvmvObjVel : IVMV_OBJVEL;
sampler sIvmvObjVel { Texture = texIvmvObjVel; MinFilter = POINT; MagFilter = POINT; MipFilter = POINT; };

texture texIvmvCurColour  { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
sampler sIvmvCurColour    { Texture = texIvmvCurColour; };
texture texIvmvPrevColour { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
sampler sIvmvPrevColour   { Texture = texIvmvPrevColour; };

// DEPTH HISTORY, and it is what makes the error view honest.
//
// Comparing this frame against the reprojected previous one lights up wherever
// the vectors are wrong. It ALSO lights up wherever the previous frame simply
// had nothing to offer, because the camera moved and revealed geometry that did
// not exist in it. That is disocclusion, it is unavoidable, and it is not a
// defect. Without a way to exclude it every pan looks like a catastrophe, which
// is exactly why the plain reprojection view could never answer the question.
//
// With the previous frame's depth in hand the test is the one a temporal filter
// uses: the vector claims where this surface was, so the depth THERE should be
// the depth this surface HAD. Where it is not, the history is a different
// surface, and the pixel is excluded rather than counted against us.
texture texIvmvCurDepth  { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = R32F; };
sampler sIvmvCurDepth    { Texture = texIvmvCurDepth; MinFilter = POINT; MagFilter = POINT; MipFilter = POINT; };
texture texIvmvPrevDepth { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = R32F; };
sampler sIvmvPrevDepth   { Texture = texIvmvPrevDepth; MinFilter = POINT; MagFilter = POINT; MipFilter = POINT; };

// A NUMBER, BECAUSE "IS IT TOO RED" IS NOT A MEASUREMENT.
//
// Every question about these vectors has come down to someone squinting at a
// picture and deciding whether it looks acceptable, and that has been wrong as
// often as right. Reducing the error over the whole frame turns it into a
// percentage that can be watched, compared between builds, and argued with.
//
// R holds 1 where a pixel is comparable AND its error exceeds the threshold.
// G holds 1 wherever a pixel is comparable at all. The ratio of their averages
// is the fraction of judgeable screen that is actually wrong, which is the only
// figure that means anything: counting the excluded pixels as correct would
// flatter the result every time the camera moves.
texture texIvmvErr { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RG16F; MipLevels = 12; };

// HARDWARE-STYLE DEPTH FOR DLSS. The game's depth buffer holds FusionShaders'
// LOGARITHMIC encoding, and DLSS5_Feed.fx used to copy those raw values into
// the DLSS depth guide. DLSS expects hardware depth in [0,1], standard or
// inverted, and uses it for disocclusion handling and vector dilation; a
// buffer where 1 m reads 0.26 instead of 0.95 is out of distribution for the
// network, worst in the near field, which is where the rider and the wheel
// ghosted. This decodes the log depth with the engine's real near/far and
// re-encodes it as REVERSE-Z, near / Z: 1 at the near plane, 0 at infinity,
// the modern default distribution and the best-conditioned one for R32F.
// DLSS5_Feed.fx declares this texture identically and writes it into
// DLSS5_Depth, with depth_inverted=1 in dlss5-feed.cfg.
texture texIvmvHwDepth { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = R32F; };
sampler sIvmvHwDepth   { Texture = texIvmvHwDepth; };
sampler sIvmvErr   { Texture = texIvmvErr; };

// ---------------------------------------------------------------------------
// Depth
// ---------------------------------------------------------------------------

// RAW depth, with ReShade's coordinate fixes applied but NONE of its decoding.
// Copied from ReShade.fxh's GetLinearizedDepth up to the point where it starts
// interpreting the value, because we decode with the engine's real near/far
// instead of ReShade's assumed far plane.
float IvmvRawDepth(float2 texcoord)
{
#if RESHADE_DEPTH_INPUT_IS_UPSIDE_DOWN
    texcoord.y = 1.0 - texcoord.y;
#endif
#if RESHADE_DEPTH_INPUT_IS_MIRRORED
    texcoord.x = 1.0 - texcoord.x;
#endif
    texcoord.x /= RESHADE_DEPTH_INPUT_X_SCALE;
    texcoord.y /= RESHADE_DEPTH_INPUT_Y_SCALE;
#if RESHADE_DEPTH_INPUT_X_PIXEL_OFFSET
    texcoord.x -= RESHADE_DEPTH_INPUT_X_PIXEL_OFFSET * BUFFER_RCP_WIDTH;
#else
    texcoord.x -= RESHADE_DEPTH_INPUT_X_OFFSET / 2.000000001;
#endif
#if RESHADE_DEPTH_INPUT_Y_PIXEL_OFFSET
    texcoord.y += RESHADE_DEPTH_INPUT_Y_PIXEL_OFFSET * BUFFER_RCP_HEIGHT;
#else
    texcoord.y += RESHADE_DEPTH_INPUT_Y_OFFSET / 2.000000001;
#endif

    float depth = tex2Dlod(ReShade::DepthBuffer, float4(texcoord, 0, 0)).x * RESHADE_DEPTH_MULTIPLIER;

#if RESHADE_DEPTH_INPUT_IS_REVERSED
    depth = 1.0 - depth;
#endif
    return depth;
}

// Raw depth -> view-space distance Z (positive, metres-ish game units).
//
// STANDARD, derived rather than copied:
//     clip.z = z_view*c + d_p          clip.w = -z_view
//     d = clip.z/clip.w,  Z = -z_view
//  => d = -c + d_p/Z   =>   Z = d_p / (d + c)
// Checked against measured values c = -1.000002, d_p = -0.05:
//     d=0 -> Z=0.05 (near),  d=1 -> Z=25000 (far).
//
// LOGARITHMIC (FusionShaders):  d = log2(Z/near)/log2(far/near)
//  => Z = near * (far/near)^d
float IvmvViewZ(float rawDepth)
{
    const float nearC = ivmv_misc.x;
    const float farC  = ivmv_misc.y;

    bool useLog = (ivmv_depth_mode == 1) ||
                  (ivmv_depth_mode == 0 && ivmv_misc.w > 0.5);

    if (useLog)
    {
        if (nearC <= 0.0 || farC <= nearC)
            return 0.0;
        return nearC * pow(farC / nearC, rawDepth);
    }

    const float c  = ivmv_proj.z;
    const float dp = ivmv_proj.w;
    float denom = rawDepth + c;
    if (abs(denom) < 1e-9)
        return farC;
    return dp / denom;
}

// ---------------------------------------------------------------------------
// The motion vector
// ---------------------------------------------------------------------------

// Returns delta-UV such that prev_uv = uv + mv. Zero when it cannot be trusted.
float2 IvmvComputeMV(float2 uv, out float outZ, out bool ok, out float outPrevZ)
{
    outZ     = 0.0;
    outPrevZ = 0.0;
    ok       = false;

    // valid==0 means the .asi did not capture a scene camera this frame, or the
    // addon is not running. Emitting a guess here would be worse than nothing:
    // a temporal filter trusts motion vectors completely.
    if (ivmv_misc.z < 0.5)
        return float2(0.0, 0.0);
    if (abs(ivmv_proj.x) < 1e-9 || abs(ivmv_proj.y) < 1e-9)
        return float2(0.0, 0.0);

    float d = IvmvRawDepth(uv);
    float Z = IvmvViewZ(d);
    outZ = Z;
    if (!(Z > 0.0) || Z > ivmv_misc.y * 1.0001)
        return float2(0.0, 0.0);

    // uv -> NDC. uv.y = 0 is the top of the screen, NDC y = +1 is the top.
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);

    // NDC + Z -> view space. No matrix needed: the projection is diagonal in
    // x and y, so this is two divides instead of a 4x4 multiply, and it is
    // better conditioned.
    float3 vpos;
    vpos.x = ndc.x * Z / ivmv_proj.x;
    vpos.y = ndc.y * Z / ivmv_proj.y;
    vpos.z = -Z;

    // Current view space -> previous view space. Row vector times row-major
    // matrix, done by hand so no packing convention can silently transpose it.
    float4 p = float4(vpos, 1.0);
    float4 pp = p.x * ivmv_reproj0
              + p.y * ivmv_reproj1
              + p.z * ivmv_reproj2
              + p.w * ivmv_reproj3;

    float prevZ = -pp.z;
    outPrevZ = prevZ;
    // Behind the previous camera, or on its near plane: there is no correct
    // answer, so say nothing rather than something plausible.
    if (prevZ <= ivmv_misc.x)
        return float2(0.0, 0.0);

    float2 prevNdc = float2(pp.x * ivmv_projPrev.x, pp.y * ivmv_projPrev.y) / prevZ;
    float2 prevUV  = float2(prevNdc.x * 0.5 + 0.5, 0.5 - prevNdc.y * 0.5);

    ok = true;
    return prevUV - uv;
}

// ---------------------------------------------------------------------------
// Passes
// ---------------------------------------------------------------------------

// Object velocity wins over the camera vector where it is valid AND its stored
// distance agrees with the scene. Disagreement means something was drawn over
// that object AFTER its velocity was written, so the stored vector belongs to a
// surface that is no longer visible at this pixel. That check is what lets the
// engine side redraw objects inline, at their own draw, instead of capturing
// buffers and replaying them once the depth buffer is complete.
float2 IvmvCombine(float2 uv, float2 cameraMv, float sceneZ, bool camOk,
                   out int verdict)
{
    verdict = 0;                       // 0 = camera, 1 = object, 2 = rejected
    float4 ov = tex2D(sIvmvObjVel, uv);
    if (ov.a < 0.5)
        return cameraMv;               // nothing was drawn here
    if (!camOk || sceneZ <= 0.0)
        return cameraMv;

    // Relative, because absolute tolerance cannot serve both a bumper 3 units
    // away and a building 800 units away.
    float rel = abs(ov.z - sceneZ) / max(sceneZ, 1e-3);
    if (rel > 0.02) { verdict = 2; return cameraMv; }

    verdict = 1;
    return ov.xy;
}

// The error at this pixel, and whether it is judgeable at all.
//   status 0  no depth, or the vector points off screen: no history existed
//   status 1  disoccluded: the history is a different surface
//   status 2  comparable, and `err` is how far outside the neighbourhood the
//             current colour falls
// NO PROTOTYPE. ReShade's FX compiler does not link one to its definition and
// reports it as an unimplemented function, so this simply sits above its first
// user instead.
void IvmvErrorAt(float2 uv, out int status, out float err, out int usedSource)
{
    status     = 0;
    err        = 0.0;
    usedSource = 0;

    // FOUR DIFFERENT THINGS WERE SHARING ONE COLOUR, and one of them is a
    // catastrophe while the others are routine. The whole screen going blue for
    // seconds at a time was read as valid = 0, but the log records only eleven
    // blank frames in the whole session and every one lasted a single frame. So
    // it is one of the others, and they have to be told apart before anything
    // can be concluded.
    //
    //   3  the .asi published nothing usable            DEEP BLUE
    //   4  the DEPTH BUFFER is unusable                 CYAN
    //   0  the vector points off screen                 SLATE
    //   1  disoccluded                                  GREY
    //
    // Cyan is the one that matters. It means ReShade has lost the game's depth
    // buffer, which it picks by heuristic from more than one candidate and has
    // never been pinned in this project. Without depth there is no camera
    // vector, so the effect emits zero for EVERY pixel, which is the same
    // catastrophe as valid = 0 arriving by a different road.
    if (ivmv_misc.z < 0.5) { status = 3; return; }
    {
        float dRaw = IvmvRawDepth(uv);
        float zRaw = IvmvViewZ(dRaw);
        if (!(zRaw > 0.0) || zRaw > ivmv_misc.y * 1.0001) { status = 4; return; }
    }

    // AND STATUS 0 WAS DOING THE SAME JOB BLUE WAS. It covered four separate
    // conditions, so a screen full of slate said nothing about which. The
    // reported colour was slate rather than cyan, which rules out the depth
    // buffer and leaves these:
    //
    //   5  GREEN   the published projection is degenerate, so there is no
    //              camera to reproject with at all
    //   6  PURPLE  every pixel reprojects to BEHIND the previous near plane,
    //              which means the reprojection matrix itself is wrong
    //   7  SLATE   the vector genuinely points off screen
    //   8  WHITE   the previous DEPTH history is empty, so nothing can be
    //              compared even though the vectors may be perfect
    //
    // Only 7 is routine. The other three are all faults, and they were
    // indistinguishable a moment ago.
    if (abs(ivmv_proj.x) < 1e-9 || abs(ivmv_proj.y) < 1e-9) { status = 5; return; }

    float z9; bool ok9; float pz9;
    float2 cam9 = IvmvComputeMV(uv, z9, ok9, pz9);
    int verdict9;
    float2 mv9 = IvmvCombine(uv, cam9, z9, ok9, verdict9);
    usedSource = verdict9;

    // valid, depth and projection are already excluded above, so the only
    // remaining way to fail is landing behind the previous near plane.
    if (!ok9 || z9 <= 0.0) { status = 6; return; }

    float2 pUV = uv + mv9;
    if (pUV.x < 0.0 || pUV.x > 1.0 || pUV.y < 0.0 || pUV.y > 1.0) { status = 7; return; }

    float prevZthere = tex2D(sIvmvPrevDepth, pUV).r;
    if (prevZthere <= 0.0) { status = 8; return; }
    if (abs(prevZthere - pz9) / max(pz9, 1e-3) > 0.05) { status = 1; return; }

    float3 cur = tex2D(sIvmvCurColour, uv).rgb;
    float2 ts  = ReShade::PixelSize;

    float3 mn = float3(1e9, 1e9, 1e9);
    float3 mx = float3(-1e9, -1e9, -1e9);
    [unroll] for (int dy = -1; dy <= 1; dy++)
    [unroll] for (int dx = -1; dx <= 1; dx++)
    {
        float3 c = tex2D(sIvmvPrevColour, pUV + float2(dx, dy) * ts).rgb;
        mn = min(mn, c);
        mx = max(mx, c);
    }

    float3 outside = max(float3(0,0,0), max(cur - mx, mn - cur));
    err    = dot(outside, float3(0.299, 0.587, 0.114));
    status = 2;
}

// R: comparable AND wrong. G: comparable at all. Averaged over the frame by the
// mip chain, R/G is the fraction of judgeable screen that is actually wrong.
float2 PS_ErrorMask(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    int status; float err; int src;
    IvmvErrorAt(uv, status, err, src);
    if (status != 2) return float2(0.0, 0.0);
    return float2(err > ivmv_err_threshold ? 1.0 : 0.0, 1.0);
}

float PS_HwDepth(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    const float raw   = IvmvRawDepth(uv);
    const float nearC = ivmv_misc.x;
    const float farC  = ivmv_misc.y;
    // Nothing published yet (first frames, a cut): no near/far to decode with.
    // Hand over an inverted copy of the raw value rather than a constant, so
    // the buffer keeps its ordering and DLSS is never fed a flat plane.
    if (nearC <= 0.0 || farC <= nearC)
        return 1.0 - saturate(raw);
    const float Z = IvmvViewZ(raw);
    if (!(Z > 0.0))
        return 0.0;                          // no depth here: treat as infinitely far
    return saturate(nearC / Z);              // reverse-Z: 1 at near, -> 0 at far
}

float2 PS_WriteMV(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float z; bool ok; float pz;
    float2 mv = IvmvComputeMV(uv, z, ok, pz);
    int verdict;
    return IvmvCombine(uv, mv, z, ok, verdict);
}

float3 PS_Display(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    float3 colour = tex2D(ReShade::BackBuffer, uv).rgb;
    if (ivmv_debug == 0)
        return colour;

    float z; bool ok; float pz;
    float2 mv = IvmvComputeMV(uv, z, ok, pz);

    if (ivmv_debug == 1)
    {
        return IvmvRawDepth(uv).xxx;
    }
    if (ivmv_debug == 2)
    {
        // LINEAR in world units, deliberately. A log-scaled view here is
        // useless: with logarithmic depth, log2(Z/near)/log2(far/near) IS the
        // raw depth by definition, so it would render identically to view 1 and
        // tell you nothing. Linear makes the decode visible: near geometry
        // should sweep black-to-white across roughly the first 200 units, and
        // everything beyond saturates.
        return saturate(z / 200.0).xxx;
    }
    if (ivmv_debug == 3)
    {
        // Red = rightward, green = downward, grey = still.
        float2 s = mv * ivmv_debug_scale * float2(BUFFER_WIDTH, BUFFER_HEIGHT) * 0.01;
        return float3(0.5 + s.x, 0.5 + s.y, ok ? 0.5 : 0.0);
    }
    if (ivmv_debug == 4)
    {
        // THE STRONGEST TEST. Sample the previous frame where this pixel says
        // it was. With a still scene this must look identical to the live
        // image; any ghosting, doubling or shear is a real error in the chain.
        return tex2D(sIvmvPrevColour, uv + mv).rgb;
    }

    if (ivmv_debug == 7)
    {
        // WHERE object velocity is actually used. Green = an object supplied
        // the vector. Red = an object wrote here but its depth disagreed with
        // the scene, so it was rejected as covered. Grey = camera vector.
        float z2; bool ok2; float pz2;
        float2 cm = IvmvComputeMV(uv, z2, ok2, pz2);
        int verdict;
        IvmvCombine(uv, cm, z2, ok2, verdict);
        if (verdict == 1) return float3(0.0, 1.0, 0.0);
        if (verdict == 2) return float3(1.0, 0.0, 0.0);
        return colour * 0.35;
    }
    if (ivmv_debug == 8)
    {
        // WHY object velocity is rejected, not merely THAT it is.
        //
        // View 7 shows a moving car almost entirely red, which means its
        // velocity was computed, written, and then discarded by the 2%
        // agreement test in IvmvCombine. That view cannot say whether the
        // disagreement is a small systematic bias (fixable) or unrelated
        // numbers (a real occlusion, correctly rejected). This one can.
        //
        // The bands ARE the tolerance we would need, so the picture answers
        // "what should the number be" directly:
        //   green   accepted today, within 2%
        //   yellow  would pass at 5%
        //   orange  would pass at 15%
        //   red     stored distance is FURTHER than the scene: something was
        //           drawn in front, which is what the test exists to catch
        //   magenta stored distance is NEARER than the scene, which occlusion
        //           cannot cause. A field of magenta means our depth or our
        //           decode is biased, not that the geometry is hidden.
        //
        // Splitting by SIGN is the whole point. Occlusion can only ever leave
        // our stored surface the nearer one, so the two colours separate
        // "working as designed" from "measuring the wrong thing".
        float4 ov8 = tex2D(sIvmvObjVel, uv);
        if (ov8.a < 0.5)
            return colour * 0.25;                  // no object wrote here
        float z8; bool ok8; float pz8;
        IvmvComputeMV(uv, z8, ok8, pz8);
        if (z8 <= 0.0)
            return float3(0.0, 0.0, 1.0);          // no scene depth to compare
        float rel8 = (ov8.z - z8) / max(z8, 1e-3);
        float a8   = abs(rel8);
        if (a8 <= 0.02) return float3(0.0, 1.0, 0.0);
        if (a8 <= 0.05) return float3(1.0, 1.0, 0.0);
        if (a8 <= 0.15) return float3(1.0, 0.5, 0.0);
        return (rel8 > 0.0) ? float3(1.0, 0.0, 0.0) : float3(1.0, 0.0, 1.0);
    }
    if (ivmv_debug == 9)
    {
        // THE ONE VIEW THAT ANSWERS "ARE THE VECTORS RIGHT".
        //
        // A motion vector makes a falsifiable claim: the surface at this pixel
        // was at uv+mv last frame. So go and look. BLACK IS THE PASS CONDITION,
        // rather than "looks about right", which is what the plain reprojection
        // view offered and why it could never settle anything.
        //
        //   black      correct
        //   red        the vector points at a different surface
        //   dark grey  disoccluded, excluded, not counted against us
        //   dark blue  no history existed here, excluded
        //   the HUD    always lights up: it has no motion vector at all
        //
        // The bar across the top is the same judgement reduced over the whole
        // frame, so it does not have to be eyeballed. See texIvmvErr.
        int status9; float err9; int src9;
        IvmvErrorAt(uv, status9, err9, src9);

        float3 outCol;
        if      (status9 == 3) outCol = float3(0.0,  0.0,  0.45);   // nothing published
        else if (status9 == 4) outCol = float3(0.0,  0.40, 0.40);   // no depth buffer
        else if (status9 == 5) outCol = float3(0.0,  0.40, 0.0);    // PROJECTION DEGENERATE
        else if (status9 == 6) outCol = float3(0.35, 0.0,  0.35);   // REPROJECTION BROKEN
        else if (status9 == 7) outCol = float3(0.10, 0.10, 0.25);   // off screen (routine)
        else if (status9 == 8) outCol = float3(0.55, 0.55, 0.55);   // NO DEPTH HISTORY
        else if (status9 == 1) outCol = float3(0.14, 0.14, 0.14);   // disoccluded
        else
        {
            // HUE SAYS WHICH VECTOR WAS USED, BRIGHTNESS SAYS HOW WRONG IT IS.
            // A wrong pixel is only half an answer. What decides the fix is
            // whether that pixel was handed the camera vector, an object vector,
            // or an object vector that was computed and then thrown away by the
            // depth-agreement test. Those are three different bugs and they were
            // indistinguishable a moment ago.
            //   RED     the camera vector was used and it is wrong. On a moving
            //           object that means its object velocity never arrived.
            //   MAGENTA an object vector was used and it is still wrong.
            //   YELLOW  an object vector was WRITTEN and then rejected, so the
            //           work was done and discarded. This is the glass case.
            float e = saturate(err9 * ivmv_debug_scale);
            if      (src9 == 1) outCol = float3(e, 0.0, e);
            else if (src9 == 2) outCol = float3(e, e * 0.85, 0.0);
            else                outCol = float3(e, e * e * 0.35, 0.0);
        }

        // --- the meter ---------------------------------------------------
        // Top mip of the reduction chain is the frame average of both channels.
        float2 avg = tex2Dlod(sIvmvErr, float4(0.5, 0.5, 0.0, 11.0)).rg;
        float  pct = (avg.g > 1e-6) ? (avg.r / avg.g) * 100.0 : 0.0;

        if (uv.y < 0.022 && uv.x < 0.5)
        {
            // Full width is 10% wrong. Ticks every 1%.
            float bar = saturate(pct / 10.0);
            float x   = uv.x / 0.5;
            if (uv.y < 0.004) return float3(0.0, 0.0, 0.0);
            if (frac(x * 10.0) < 0.006) return float3(0.35, 0.35, 0.35);
            if (x <= bar)
                return pct < 1.0  ? float3(0.0, 0.8, 0.1)
                     : pct < 3.0  ? float3(0.9, 0.8, 0.0)
                                  : float3(1.0, 0.15, 0.0);
            return float3(0.05, 0.05, 0.05);
        }
        return outCol;
    }
    if (ivmv_debug == 6)
    {
        // CHAIN TEST. The .asi clears its engine-side target to magenta every
        // frame. Magenta here means creation, DXVK Vulkan interop, hand-off
        // through shared memory, view creation, semantic binding AND the image
        // layout transition are ALL working. Black means one of them is not,
        // and the two logs say which. Testing only whether
        // create_resource_view returned success would prove far less: it can
        // succeed while the shader still samples nothing.
        return tex2D(sIvmvObjVel, uv).rgb;
    }

    // Diagnostics: corner bars that make a dead feed obvious at a glance.
    if (uv.y < 0.02)
    {
        if (uv.x < 0.25) return ivmv_misc.z > 0.5 ? float3(0,1,0) : float3(1,0,0);
        if (uv.x < 0.50) return ivmv_misc.w > 0.5 ? float3(0,0.6,1) : float3(0.3,0.3,0.3);
        // A moving stripe: frozen means the addon is not updating.
        if (uv.x < 0.75) return frac(ivmv_frame / 60.0).xxx;
        return abs(ivmv_proj.x) > 1e-9 ? float3(1,1,0) : float3(0.2,0,0);
    }
    return colour;
}

float PS_SaveDepth(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // Linear view-space distance, matching what IvmvComputeMV reasons about, so
    // the disocclusion test compares like with like.
    return IvmvViewZ(IvmvRawDepth(uv));
}

float PS_RollDepth(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return tex2D(sIvmvCurDepth, uv).r;
}

float3 PS_SaveCurrent(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    // The PRISTINE frame, captured before the display pass touches it.
    return tex2D(ReShade::BackBuffer, uv).rgb;
}

float3 PS_RollPrevious(float4 vpos : SV_Position, float2 uv : TEXCOORD) : SV_Target
{
    return tex2D(sIvmvCurColour, uv).rgb;
}

technique IVMotionVectors <
    ui_label   = "IVMotionVectors (geometric)";
    ui_tooltip = "Real geometric motion vectors from the GTA IV engine's own "
                 "camera matrices. Must run BEFORE DLSS5_Feed.fx. Requires "
                 "IVMotionVectors.asi and IVMotionVectors.addon32.";
>
{
    pass WriteMotionVectors
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_WriteMV;
        RenderTarget = texMotionVectors;
    }
    pass WriteHwDepth
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_HwDepth;
        RenderTarget = texIvmvHwDepth;
    }
    // ORDER IS LOAD-BEARING. Snapshot the untouched frame, THEN display, THEN
    // roll the snapshot into history. Saving after the display pass feeds our
    // own output back in and the reprojection test compounds its own error.
    pass SaveCurrentColour
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_SaveCurrent;
        RenderTarget = texIvmvCurColour;
    }
    pass SaveCurrentDepth
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_SaveDepth;
        RenderTarget = texIvmvCurDepth;
    }
    pass ComputeErrorMask
    {
        VertexShader    = PostProcessVS;
        PixelShader     = PS_ErrorMask;
        RenderTarget    = texIvmvErr;
        GenerateMipMaps = true;
    }
    pass Display
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_Display;
    }
    pass RollPrevious
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_RollPrevious;
        RenderTarget = texIvmvPrevColour;
    }
    pass RollPreviousDepth
    {
        VertexShader = PostProcessVS;
        PixelShader  = PS_RollDepth;
        RenderTarget = texIvmvPrevDepth;
    }
}
