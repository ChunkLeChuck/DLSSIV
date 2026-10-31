// THE DITHER PASS'S FIRST FORM, KEPT AS THE PROOF'S REFERENCE (tools/test-dedither, not the mod's): the shader as it
// was until 2026-10-03 late, which wrote a whole copy of the scene with the changed pixels in it. The shipping pass
// writes only the changed pixels into the scene buffer itself, and proof 9 holds its picture to this one's, bit for
// bit. build.ps1 compiles it into %TEMP% for dd.cpp; nothing of it ships. Everything below is that shader, unchanged.
//
// THE GAME'S DITHER, SMOOTHED BEFORE THE UPSCALER (2026-10-03, the owner: "release 1.0.8.1 after the dither fix").
//
// WHAT THE GAME DOES. A see-through or fading object is drawn into the G-buffer with pixels left out by a 2x2 pattern
// of the pixel's own screen position (FusionFix's deferred passes: thresholds 0.2, 0.4, 0.6, 0.8 by the parity of x
// and y), and the pass marks the pixels it did draw in the alpha of the third G-buffer target. FusionFix's post chain
// opens with a filter that reads that mark and blends the pattern away. This mod runs the upscaler BEFORE the post
// chain, so the upscaler was given the raw pattern: tied to the pixel grid while everything else in the picture moves
// with the jitter, it is the one thing the upscaler cannot settle, and it came out crawling (a player's report on
// 1.0.7 and 1.0.8, worse with FusionFix's depth of field; docs/DECISIONS.md, 2026-10-03, 09:00).
//
// WHAT THIS PASS DOES. The same decision FusionFix's filter makes, made here, at the render size, where the scene and
// the mark are still on one pixel grid and the pattern is exact. The upscaler is then given what the unmodded game's
// post chain works on after its filter. Written for this mod from the rule, not from FusionFix's text:
//
//   THE MARK   the mask's alpha at this pixel, at its right and lower neighbours and at the one diagonally below:
//              any of them set and the pixel may be part of a pattern. ONE TAP reads the four: it sits on the corner
//              they share and the sampler averages them, so the sum is above zero exactly when one is set. No mark,
//              no change, and that is nearly every pixel: two fetches, and the branch below is not taken.
//   THE TEST   four taps, each on the corner four pixels share, so each is the mean of a 2x2 block; the blocks stand
//              in a pinwheel around this pixel and none holds it. A 2x2 block holds each of the pattern's four
//              phases once, so every tap is the properly blended colour. The pixel is the odd one out when its luma
//              is further from the taps' mean than the taps are from each other, squared and summed.
//   THE WRITE  mode 1: the mean of the four taps (sixteen pixels around this one; FusionFix's footprint).
//              mode 2: the mean of the four 2x2 blocks that DO hold this pixel, which is the 3x3 tent 1-2-1: the
//              smallest window that weighs the four phases equally, so the same blend with less blur.
//
//   s0  the scene, the render size, LINEAR filtering (the taps on shared corners rely on it)
//   s1  the third G-buffer target, the same size, LINEAR (the one tap on the shared corner): alpha is the mark
//   c0  xy = one texel; z = the mode (1 or 2)
//
// The view variant (ivt_dedither_view_ps.hlsl, development builds) paints what this pass decides instead of writing
// it: magenta where a pixel would be moved, a green tint where the mark was set and nothing moves.
sampler2D sScene : register(s0);
sampler2D sMask  : register(s1);
float4    gDed   : register(c0);

static const float3 kLuma = float3(0.2125, 0.7154, 0.0721);

float3 Tap(float2 uv) { return tex2Dlod(sScene, float4(uv, 0.0, 0.0)).rgb; }

void main(float2 vpos : VPOS, out float4 outColor : COLOR0)
{
    const float2 inv = gDed.xy;
    const float2 uv  = (vpos + 0.5) * inv;      // VPOS is the integer position
    const float4 c   = tex2Dlod(sScene, float4(uv, 0.0, 0.0));
    outColor = c;
    const float mark = tex2Dlod(sMask, float4(uv + 0.5 * inv, 0.0, 0.0)).a;
    [branch] if (mark > 0.0)
    {
        const float3 t0 = Tap(uv + float2(-0.5, -1.5) * inv);
        const float3 t1 = Tap(uv + float2( 1.5, -0.5) * inv);
        const float3 t2 = Tap(uv + float2( 0.5,  1.5) * inv);
        const float3 t3 = Tap(uv + float2(-1.5,  0.5) * inv);
        const float4 lt = float4(dot(t0, kLuma), dot(t1, kLuma), dot(t2, kLuma), dot(t3, kLuma));
        const float  mean = dot(lt, 0.25);
        const float4 d  = lt - mean;
        const float  dc = dot(c.rgb, kLuma) - mean;
        const bool odd = dc * dc >= dot(d, d);
#ifdef DED_VIEW
        // MAGENTA ONLY WHERE THE WRITE WOULD MOVE THE PIXEL (by more than a hundredth of it in some channel): a marked
        // surface of one flat colour passes the test with nothing to change, and painting it would say the pass does
        // more than it does. Judged on mode 1's write.
        const float3 gap = abs((t0 + t1 + t2 + t3) * 0.25 - c.rgb) - 0.01 * (abs(c.rgb) + 0.01);
        const bool moves = odd && max(gap.r, max(gap.g, gap.b)) > 0.0;
        outColor.rgb = moves ? float3(4.0, 0.0, 4.0) : lerp(c.rgb, float3(0.0, 2.0, 0.0), 0.5);
#else
        [branch] if (odd)
        {
            float3 blend = (t0 + t1 + t2 + t3) * 0.25;
            [branch] if (gDed.z > 1.5)
                blend = (Tap(uv + float2(-0.5, -0.5) * inv) + Tap(uv + float2( 0.5, -0.5) * inv)
                       + Tap(uv + float2(-0.5,  0.5) * inv) + Tap(uv + float2( 0.5,  0.5) * inv)) * 0.25;
            outColor.rgb = blend;
        }
#endif
    }
}
