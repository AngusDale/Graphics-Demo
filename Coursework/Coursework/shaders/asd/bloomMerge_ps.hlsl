Texture2D texture1 : register(t0);
Texture2D texture2 : register(t1);
SamplerState Sampler0 : register(s0);

cbuffer BloomBuffer : register(b0)
{
    float intensity;
    float3 padding;
}

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};


float4 main(InputType input) : SV_TARGET
{
    
    float4 tex1 = texture1.Sample(Sampler0, input.tex);
    float4 tex2 = texture2.Sample(Sampler0, input.tex);
    
    float4 colour = float4(tex1.rgb + tex2.rgb * intensity, 1.0f);
    return colour;
    
}