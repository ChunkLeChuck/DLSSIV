// THE PROOF'S OWN READER of a corona's brightness (tools/test-corona, not the mod's): the corona vertex shader hands
// its brightness times the vertex colour to the pixel shader in TEXCOORD1; the tool draws white coronas, so this
// writes the brightness itself where the sprite is, and nothing where the shader collapsed it. Nothing of this ships.
float4 main(float3 col : TEXCOORD1) : COLOR0
{
    return float4(col.x, 0.0, 0.0, 1.0);
}
