// A stand-in for the game's distance pass: the scene read at the quad's coordinate and written back changed. The
// game's own applies desaturation, a gamma and fog by depth; a gain is enough to see a pixel that was read after it
// had already been written.
sampler2D gScene : register(s0);
float4 gGain : register(c0);
float4 main(float2 uv : TEXCOORD0) : COLOR0
{
    float4 c = tex2D(gScene, uv);
    return float4(c.rgb * gGain.x, 1.0);
}
