struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

VsOut main(float4 position : POSITION, float2 uv : TEXCOORD)
{
    VsOut vOut;
    vOut.position = position;
	vOut.uv = uv;
    return vOut;
}