Texture2D shaderTexture : register(t0);
SamplerState SampleType : register(s0);

cbuffer ScreenSizeBuffer : register(b0)
{
    float screenWidth;
    float3 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
};

float4 main(InputType input) : SV_TARGET
{
    float weight[6];
    float2 blurCoords[11];
    float4 colour;

	// Create the weights that each neighbor pixel will contribute to the blur.   
    // 0.2476614 + 0.230713 + 0.0321703 + 0.075984 + 0.028002 + 0.0093 + 0.230713 + 0.0321703 + 0.075984 + 0.028002 + 0.0093 = 1
    
    // kernel size of 11
    weight[0] = 0.2476614;
    weight[1] = 0.230713;
    weight[2] = 0.075984;
    weight[3] = 0.0321703;
    weight[4] = 0.028002;
    weight[5] = 0.0093;

    // Initialize the colour to black.
    colour = float4(0.0f, 0.0f, 0.0f, 0.0f);

    float texelSize = 1.0f / screenWidth;
    // Add the vertical pixels to the colour by the specific weight of each.
    
    // set the blur coordinates
    for (int i = -5; i <= 5; i++)
    {
        blurCoords[i + 5] = input.tex + float2(texelSize * i, 0.0f);
    }
    
    // set the colour by adding the neightboring texels
    for (int j = -5; j <= 5; j++)
    {
        colour += shaderTexture.Sample(SampleType, blurCoords[j + 5]) * weight[abs(j)];
    }
    
	// Set the alpha channel to one.
    colour.a = 1.0f;
    return colour;
}
