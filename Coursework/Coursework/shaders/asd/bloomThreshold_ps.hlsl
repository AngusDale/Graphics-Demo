
Texture2D texture0 : register(t0);
SamplerState Sampler0 : register(s0);

cbuffer ThresholdBuffer : register(b0)
{
    float threshold;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};


float4 main(InputType input) : SV_TARGET
{
    float4 textureColour = texture0.Sample(Sampler0, input.tex);
    float lightLevel = 0;
    
    for (int i = 0; i < 3; i++)
    {
        lightLevel += textureColour[i];
    }
    
    if (lightLevel > threshold)
    {    
        return textureColour;
    }
    
    return float4(0, 0, 0, 1.0f);

}