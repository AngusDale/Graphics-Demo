// Tessellation Hull Shader
// Prepares control points for tessellation

cbuffer CameraBuffer : register(b0)
{
    float3 camPos;
    float pad;    
};

struct InputType
{
    float3 position : POSITION;
};

struct ConstantOutputType
{
    float edges[4] : SV_TessFactor;
    float inside[2] : SV_InsideTessFactor;
};

struct OutputType
{
    float3 position : POSITION;
};

ConstantOutputType PatchConstantFunction(InputPatch<InputType, 4> inputPatch, uint patchId : SV_PrimitiveID)
{    
    ConstantOutputType output;
    
    float far = 20;
    float3 near = camPos;
    float dist = 0.0f;
    
    float3 avg_pos = 0.0f;
    for (int x = 0; x < 4; x++)
    {
        avg_pos += inputPatch[x].position.xyz;
    }
    avg_pos /= 4;   
    
    dist = distance(camPos, avg_pos);
    float a = smoothstep(0, far, dist);
    float inside = lerp(16, 1, a);
    
    output.inside[0] = inside; // collumns
    output.inside[1] = inside; // rows
    
    
    float3 pos[4];
    pos[0] = (inputPatch[0].position.xyz + inputPatch[1].position.xyz) / 2.0f;
    pos[1] = (inputPatch[0].position.xyz + inputPatch[3].position.xyz) / 2.0f;
    pos[2] = (inputPatch[2].position.xyz + inputPatch[3].position.xyz) / 2.0f;
    pos[3] = (inputPatch[1].position.xyz + inputPatch[2].position.xyz) / 2.0f;       
    
    for (int i = 0; i < 4; i++)
    {
        dist = distance(camPos, pos[i]);
        float a = smoothstep(0, far, dist);
        output.edges[i] = lerp(15, 1, a);
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
    
    return output;
}