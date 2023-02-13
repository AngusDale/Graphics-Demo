// Tessellation domain shader
// After tessellation the domain shader processes the all the vertices

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix[4][6];
    matrix lightProjectionMatrix[4];
};

cbuffer CameraBuffer : register(b1)
{
    float3 cameraPosition;
    float padding;
}

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct InputType
{
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 viewVector : TEXCOORD1;
    float3 worldPosition : TEXCOORD2;
    float4 lightViewPos[4][6] : TEXCOORD3;
};

[domain("quad")]
OutputType main(ConstantOutputType input, float2 uvwCoord : SV_DomainLocation, const OutputPatch<InputType, 4> patch)
{
    float3 vertexPosition;
    float2 texCoord;
    float3 normal;
    OutputType output;
 
    // Determine the position of the new vertex.
	// Invert the y and Z components of uvwCoord as these coords are generated in UV space and therefore y is positive downward.
	// Alternatively you can set the output topology of the hull shader to cw instead of ccw (or vice versa).
	//vertexPosition = uvwCoord.x * patch[0].position + -uvwCoord.y * patch[1].position + -uvwCoord.z * patch[2].position;
    float3 v1 = lerp(patch[0].position, patch[1].position, uvwCoord.y);
    float3 v2 = lerp(patch[3].position, patch[2].position, uvwCoord.y);
    vertexPosition = lerp(v1, v2, uvwCoord.x);
    
    float2 t1 = lerp(patch[0].tex, patch[1].tex, uvwCoord.y);
    float2 t2 = lerp(patch[3].tex, patch[2].tex, uvwCoord.y);
    texCoord = lerp(t1, t2, uvwCoord.x);
    
    float3 n1 = lerp(patch[0].normal, patch[1].normal, uvwCoord.y);
    float3 n2 = lerp(patch[3].normal, patch[2].normal, uvwCoord.y);
    normal = lerp(n1, n2, uvwCoord.x);
    
    // Calculate the position of the new vertex against the world, view, and projection matrices.
    output.position = mul(float4(vertexPosition, 1.0f), worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    // Calculate the position of the vertice as viewed by the light source.
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            output.lightViewPos[i][j] = mul(float4(vertexPosition, 1.0f), worldMatrix);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightViewMatrix[i][j]);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightProjectionMatrix[i]);
        }
    }
    
    // Calculate the normal vector against the world matrix only and normalise.
    output.normal = mul(normal, (float3x3) worldMatrix);
    output.normal = normalize(output.normal);
    
    output.tex = texCoord;
    
    output.worldPosition = mul(float4(vertexPosition, 1.0f), worldMatrix).xyz;
    
    output.viewVector = normalize(cameraPosition.xyz - output.worldPosition.xyz);

    return output;
}

