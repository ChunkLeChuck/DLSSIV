// The game's full-screen vertex shader in what matters here: position and texture coordinate passed straight through
// (read from a RenderDoc capture, docs/DECISIONS.md, "THE FAINT LINES UNDER A RENDER SCALE").
struct VSIn  { float3 pos : POSITION; float2 uv : TEXCOORD0; };
struct VSOut { float4 pos : POSITION; float2 uv : TEXCOORD0; };
VSOut main(VSIn i)
{
    VSOut o;
    o.pos = float4(i.pos, 1.0);
    o.uv = i.uv;
    return o;
}
