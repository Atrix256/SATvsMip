Texture2D gTex;
SamplerState gSampler;

struct VsOut
{
    float4 position : POSITION;
    float2 uv       : TEXCOORD;
};

float4 main(VsOut vsout) : SV_TARGET
{
    return float4(vsout.position.rgb, 1.0f);
}