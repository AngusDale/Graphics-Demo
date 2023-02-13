// Tessellation domain shader
// After tessellation the domain shader processes the all the vertices

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer TimerBuffer : register(b1)
{
    float time;
    float planeToSphere;
    float height;
    int pad;
    
    float2 amplitude;
    float2 frequency;
    float2 speed;
    float2 pad1;
    float4 spherePos;
    
    bool dynamicTess;
    float3 pad2;
}

cbuffer CameraBuffer : register(b2)
{
    float3 cameraPosition;
    float padding1;
}

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

float getHeight(float2 uv)
{
    float offset = texture0.SampleLevel(sampler0, uv, 0).r;
    return offset * height;
}

float getWaveOffset(float2 pos)
{
    float offset = ((sin((pos.x * frequency.x) + (time * speed.x)) * amplitude.x) + (sin((pos.y * frequency.y) + (time * speed.y)) * amplitude.y)) / 2.0f;
    return offset;
}

float3 pointToSphere(float3 pos, float2 uv, float3 center)
{
    float lon, lat, r;
    float radius = spherePos.w;
    
    float pi = 3.14159265359;
    
    // remove the gap in the seam where the plane edges meet
    float resolution = 1 / (30.0f - 1.0f);
    uv.x *= 1 + resolution;
    uv.y *= 1 + resolution;
    // convert UV space to logitude and latitude
    lon = pi * (uv.x - 0.25f) * 2; // offset the x by -0.25 change the position of the seam
    lat = pi * (uv.y - 0.5);
        
    r = radius + pos.y;
    pos.x = -r * cos(lat) * cos(lon) + center.x;
    pos.y = r * cos(lat) * sin(lon) + center.y;
    pos.z = r * sin(lat) + center.z;
    
    return pos;
}

[domain("quad")]
OutputType main(ConstantOutputType input, float2 uvwCoord : SV_DomainLocation, const OutputPatch<InputType, 4> patch)
{
    float3 vertexPosition;
    float2 texCoord;
    float3 normal;
    OutputType output;
 
    // Determine the position of the new vertex.
    float3 v1 = lerp(patch[0].position, patch[1].position, uvwCoord.y);
    float3 v2 = lerp(patch[3].position, patch[2].position, uvwCoord.y);
    vertexPosition = lerp(v1, v2, uvwCoord.x);
    
    float2 t1 = lerp(patch[0].tex, patch[1].tex, uvwCoord.y);
    float2 t2 = lerp(patch[3].tex, patch[2].tex, uvwCoord.y);
    texCoord = lerp(t1, t2, uvwCoord.x);
    
    // -- VERTEX MANIPULATION
    float3 center = spherePos.xyz;
	// calculate height of vertex
    float4 textureColour = texture0.SampleLevel(sampler0, texCoord, 0);
    // calculate wave offset 
    float offset = getWaveOffset(vertexPosition.xz);
    // adjust the position by the texture and offset
    vertexPosition.y = height * textureColour.r + offset;
    // get the position of the vertex, lerp'd betweek the flat and spherical plane
    vertexPosition = lerp(vertexPosition, pointToSphere(vertexPosition, texCoord, center), planeToSphere);
    
    // Calculate the position of the new vertex against the world, view, and projection matrices.
    output.position = mul(float4(vertexPosition, 1.0f), worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    output.depthPosition = output.position;

    return output;
}

