StructuredBuffer<float4x4> PerObjects : register(t0);

cbuffer Draw : register(b0)
{
    uint objectIndex;
    uint pad0;
    uint pad1;
    uint pad2;
    float2 uvOffset;
    float2 uvScale;
};

struct VS_INPUT
{
    float3 inPos : POSITION;
    float2 inTexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 outPosition : SV_POSITION;
    float2 outTexCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    float4 worldPos = float4(input.inPos, 1.0f);
    output.outPosition = mul(worldPos, PerObjects[objectIndex]);
    output.outTexCoord = input.inTexCoord;
    return output;
}
