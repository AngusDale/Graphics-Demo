// Texture pixel/fragment shader
// Basic fragment shader for rendering textured geometry

// Texture and sampler registers
Texture2D texture0 : register(t0);
SamplerState Sampler0 : register(s0);

cbuffer ToneBuffer : register(b0)
{
    float exposure;
    bool gammaCorrection;
    bool pad;
    float2 padding;
}

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};


float4 main(InputType input) : SV_TARGET
{
	// Sample the pixel color from the texture using the sampler at this texture coordinate location.
    float4 textureColour = texture0.Sample(Sampler0, input.tex);
    
    // apply tone mapping
    // https://learnopengl.com/Advanced-Lighting/HDR
    
    float gamma = 2.2f;
    float3 colour = textureColour.rgb;    
    // reinhard tone mapping
    colour *= exposure;
    colour = colour / (colour + 1.0f);
    // gamma correction
    if(gammaCorrection)
        colour = pow(colour, gamma);
    
    // return the LDR colour
    return saturate(float4(colour, 1.0f));
    
}