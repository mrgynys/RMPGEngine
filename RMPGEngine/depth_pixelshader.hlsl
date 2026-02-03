Texture2D objTexture : register(t0);
SamplerState objSamplerState : register(s0);

struct PS_INPUT
{
    float4 outPosition : SV_POSITION;
    float4 depthPosition : TEXTURE0;
    float2 outTexCoord : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_TARGET
{
    float4 textureColor = objTexture.Sample(objSamplerState, input.outTexCoord);
    
    if (textureColor.a < 0.5f)
    {
        discard;
    }
    
    float depthValue = input.depthPosition.z / input.depthPosition.w;
    return float4(depthValue, depthValue, depthValue, 1.0f);
}