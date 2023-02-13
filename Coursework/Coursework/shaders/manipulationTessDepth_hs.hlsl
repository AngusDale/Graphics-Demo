// Tessellation Hull Shader
// Prepares control points for tessellation
Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer TimerBuffer : register(b0)
{
    float time;
    float planeToSphere;
    float height;
    int tessellationFactor;
    
    float2 amplitude;
    float2 frequency;
    float2 speed;
    float2 pad0;
    float4 spherePos;
    
    bool dynamicTess;
    int edgeTessFactor;
    float nearBound;
    float farBound;
}

cbuffer CameraBuffer : register(b1)
{
    float3 camPos;
    float pad2;
};

cbuffer MatrixBuffer : register(b3)
{
    matrix worldMatrix;
}

struct InputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct OutputType
{
    float3 position : POSITION;
    float2 tex : TEXCOORD0;
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

float3 manipulateVertex(float3 vertexPosition, float2 texCoords)
{
    // -- VERTEX MANIPULATION
    float3 center = spherePos.xyz;
	// calculate height of vertex
    float4 textureColour = texture0.SampleLevel(sampler0, texCoords, 0);
    // calculate wave offset 
    float offset = getWaveOffset(vertexPosition.xz);
    // adjust the position by the texture and offset
    vertexPosition.y = height * textureColour.r + offset;
    vertexPosition = lerp(vertexPosition, pointToSphere(vertexPosition, texCoords, center), planeToSphere);
    return vertexPosition;
}

ConstantOutputType PatchConstantFunction(InputPatch<InputType, 4> inputPatch, uint patchId : SV_PrimitiveID)
{    
    ConstantOutputType output;
    
    // there's a bug somewhere that is offsetting the camera position. This is a bandaid fix.
    float3 cameraPos = camPos - float3(-15, -5, -15);
    
    // if dynamic tessellation is enabled
    if (dynamicTess)
    {
        float dist = 0.0f;
        float3 new_position[4];
        
        // manipulate the new verticies to ensure that the dynamic tesselation works off of the current vertex position
        for (int j = 0; j < 4; j++)
        {
            new_position[j] = manipulateVertex(inputPatch[j].position, inputPatch[j].tex);
        }
    
        // calculate the inside tessellation
        float3 avg_pos = 0.0f;
        
        for (int x = 0; x < 4; x++)
        {
            // find the average position of the patch 
            avg_pos += new_position[x];
        }
        avg_pos /= 4;
        
        // get the distance between the camera and the average positon
        dist = distance(cameraPos, avg_pos);
        // find where that position is between the near and far bound
        float a = smoothstep(nearBound, farBound, dist);
        // lerp between the max and min tessellation values
        float inside = lerp(10, 1, a);
    
        output.inside[0] = inside; // collumns
        output.inside[1] = inside; // rows    
    
        // calculate the edge tessellation based on the average position of 2 patches
        float3 pos[4];
        pos[0] = (new_position[0] + new_position[1]) / 2.0f;
        pos[1] = (new_position[0] + new_position[3]) / 2.0f;
        pos[2] = (new_position[2] + new_position[3]) / 2.0f;
        pos[3] = (new_position[1] + new_position[2]) / 2.0f;
    
        for (int i = 0; i < 4; i++)
        {
            // get the distance between the camera and the average positon            
            dist = distance(cameraPos, pos[i]);
            // find where that position is between the near and far bound            
            float a = smoothstep(nearBound, farBound, dist);
            // lerp between the max and min tessellation values
            output.edges[i] = lerp(10, 1, a);
        }
    }
    else
    {
        output.inside[0] = tessellationFactor; // collumns
        output.inside[1] = tessellationFactor; // rows
        
        
        for (int i = 0; i < 4; i++)
        {
            output.edges[i] = edgeTessFactor;
        }
        
    }
    return output;   
    
}


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
[patchconstantfunc("PatchConstantFunction")]
OutputType main(InputPatch<InputType, 4> patch, uint pointId : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    OutputType output;

    // Set the position for this control point as the output position.
    output.position = patch[pointId].position;
    // Set the input colour as the output colour.
    output.tex = patch[pointId].tex;
    return output;
}