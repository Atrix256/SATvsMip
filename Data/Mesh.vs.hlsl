cbuffer PerFrameCB
{
    float4x4 vpMtx;
};

struct VsOut
{
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

VsOut main(float4 position : POSITION, float2 uv : TEXCOORD)
{
    VsOut vOut;
    vOut.position = mul(float4(position.xyz, 1.0f), vpMtx);
	vOut.uv = uv;
    return vOut;
}