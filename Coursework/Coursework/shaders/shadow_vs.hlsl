
cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix[4][6];
    matrix lightProjectionMatrix[4];
};

cbuffer CameraBuffer : register(b2)
{
    float3 cameraPosition;
    float pad2;
}

struct InputType
{
    float4 position : POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewVector : TEXCOORD1;    
    float3 worldPosition : TEXCOORD2;    
    float4 lightViewPos[4][6] : TEXCOORD3;
};


OutputType main(InputType input)
{
    OutputType output;

	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
	// Calculate the position of the vertex as viewed by the light source.
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            output.lightViewPos[i][j] = mul(input.position, worldMatrix);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightViewMatrix[i][j]);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightProjectionMatrix[i]);
        }
    }
    
    output.tex = input.tex;
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
    
    output.worldPosition = mul(input.position, worldMatrix).xyz;
    
    output.viewVector = normalize(cameraPosition.xyz - output.worldPosition.xyz);
    
    return output;
}