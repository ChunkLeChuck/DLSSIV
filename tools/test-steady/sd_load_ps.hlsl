// THE PROOF'S OWN LOADER (tools/test-steady, not the mod's): it puts a known depth picture into an INTZ depth texture,
// which cannot be locked and written from the CPU. Drawn with the INTZ as the depth-stencil, the test set to always
// pass, depth writes on and colour writes off. build.ps1 compiles it into %TEMP% for sd.cpp; nothing of it ships.
//
//   s0  the picture (an R32F texture written from the CPU); POINT
//   c0  xy = one texel
sampler2D sPic : register(s0);
float4    gSd  : register(c0);

void main(float2 vpos : VPOS, out float4 col : COLOR0, out float dep : DEPTH)
{
    const float v = tex2Dlod(sPic, float4((vpos + 0.5) * gSd.xy, 0.0, 0.0)).r;
    col = float4(v, 0.0, 0.0, 1.0);
    dep = v;
}
