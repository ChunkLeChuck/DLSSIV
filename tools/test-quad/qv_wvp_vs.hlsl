// A vertex shader that TRANSFORMS its position, as the game's coronas, particles and sky do with the same immediate
// vertex: what QuvVsStraight must turn away.
float4x4 gWorldViewProj : register(c8);
struct VSIn  { float3 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn i)
{
    VSOut o;
    o.pos = mul(float4(i.pos, 1.0), gWorldViewProj);
    o.uv = i.uv;
    return o;
}
