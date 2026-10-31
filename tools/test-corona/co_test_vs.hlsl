// THE PROOF'S OWN PORT of the visibility test in the game's corona vertex shader (FusionFix's deferred_lighting,
// technique corona, VS11), instruction for instruction where the test is concerned, so that tools/test-corona can run
// on a machine without FusionFix's sources. Where they are at hand (co.exe --vs11 <path>) the tool assembles the real
// shader and runs both, and they must agree. Nothing of this ships.
//
// The same registers as the real one: c8..c11 gWorldViewProj, c15 the camera's position, c44 the screen size and its
// reciprocal, c208..c219 poisson12, c227 FusionFix's logarithmic depth pair, s0 the depth (the first vertex sampler).
// The real shader's two other branches (the camera inside the corona; the fog) are not here: the tool keeps the
// camera outside and the fog off.
float4    gRows[4]     : register(c8);
float4    gScreen      : register(c44);
float4    gPoisson[12] : register(c208);
float4    gLog         : register(c227);
sampler2D sDepth       : register(s0);

struct VI { float3 pos : POSITION; float4 col : COLOR0; float2 uv : TEXCOORD0; float3 centre : NORMAL; };
struct VO { float4 pos : POSITION; float2 uv : TEXCOORD0; float3 col : TEXCOORD1; };

VO main(VI i)
{
    const float3 off = i.pos - i.centre;
    const float4 c = i.centre.x * gRows[0] + i.centre.y * gRows[1] + i.centre.z * gRows[2] + gRows[3];
    const float dz = log2(c.w * gLog.x) * gLog.y;
    const float2 ndc = c.xy * (1.0 / c.w);
    float vis = 0.0;
    [unroll] for (int k = 0; k < 12; k++)     // twelve texldl, as the real one has them (the mod classifies a shader by that count)
    {
        float2 p = gScreen.zw * gPoisson[k].xy * 2.0 + ndc;
        p = min(max(p, -0.996999979), 0.996999979);
        const float s = tex2Dlod(sDepth, float4(p * float2(0.5, -0.5) + 0.5, 0.0, 0.0)).x;
        vis += ((dz < s) ? 1.0 : 0.0) * 0.0833333358;
    }
    const float3 wp = ((0.00999999978 < vis) ? 1.0 : 0.0) * off + i.centre;
    VO o;
    o.pos = wp.x * gRows[0] + wp.y * gRows[1] + wp.z * gRows[2] + gRows[3];
    o.col = vis * i.col.rgb * i.uv.y;
    o.uv  = float2(i.uv.x, i.col.a);
    return o;
}
