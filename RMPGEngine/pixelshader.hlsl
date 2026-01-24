cbuffer Draw : register(b0)
{
    uint objectIndex;
    uint padding0;
    uint padding1;
    uint padding2;
    float2 uvOffset;
    float2 uvScale;
}

struct PS_INPUT
{
    float4 inPosition : SV_POSITION;
    float2 inTexCoord : TEXCOORD;
};

Texture2D objTexture : TEXTURE : register(t0);
SamplerState objSamplerState : SAMPLER : register(s0);

float4 main(PS_INPUT input): SV_TARGET
{
    float2 uv = input.inTexCoord * uvScale + uvOffset;
    float4 pixelColor = objTexture.Sample(objSamplerState, uv);
    return pixelColor;
}
