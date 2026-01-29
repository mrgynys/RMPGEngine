cbuffer Draw : register(b0)
{
    uint objectIndex;
    uint padding0;
    uint padding1;
    uint padding2;
    float2 uvOffset;
    float2 uvScale;
    float4 tintColor;
    float tintIntensity;
    float pad3;
    float pad4;
    float pad5;
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
    float originalAlpha = pixelColor.a;
    pixelColor.rgb = lerp(pixelColor.rgb, tintColor.rgb, tintIntensity);
    pixelColor.a = originalAlpha * tintColor.a;
    return pixelColor;
}
