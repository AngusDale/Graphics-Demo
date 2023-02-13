// triangle_gs
// Geometry shader that generates a triangle for every vertex.

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
};

cbuffer WindBuffer : register(b1)
{
    float2 amplitude;
    float2 frequency;
    float2 speed;
    float time;
    bool surfaceLighting;
    float planeToSphere;
    float3 padding;
}

cbuffer PositionBuffer
{
    static float3 g_position[3] =
    {
        float3(-1.0f, 0, 0),
        float3(1.0f, 0, 0),
        float3(0, 1.0f, 0)
    };
};

struct InputType
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
};

struct OutputType
{
    float4 position : SV_POSITION;
    float4 depthPosition : TEXCOORD0;
};

// generate pseudo random numbers
float rand(float2 co)
{
    return 0.5 + (frac(sin(dot(co.xy, float2(12.9898, 78.233))) * 43758.5453)) * 0.5;
}

float3 rotateUnitVector(float3 unitVector, float3 newSurfaceNormal)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    
    float3 a = normalize(up); // vector a
    float3 b = normalize(newSurfaceNormal); // to be rotated to vector b
    float3 u = normalize(cross(newSurfaceNormal, up)); // axis of rotation   
    
    float c = dot(a, b); // cosine of the angle
    float angle = acos(c);
    float s = sin(angle); // sine of the angle
    
    // identity matrix
    float3x3 identity = float3x3(
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1));
    
    // axis rotation
    float3x3 axisMatrix = float3x3(
    float3(0, -u.z, u.y),
    float3(u.z, 0, -u.x),
    float3(-u.y, u.x, 0));
    
    // https://math.stackexchange.com/questions/142821/matrix-for-rotation-around-a-vector
    float3x3 rotationMatrix = identity + (axisMatrix * s) + (mul(axisMatrix, axisMatrix) * ((1 - c) / pow(s, 2)));
    
    // return the normalised rotated vector
    return normalize(mul(unitVector, rotationMatrix));
}

// rotate a normalised vector about an axis with a known angle
float3 rotateByAngle(float3 unitVector, float angle, float3 rotationAxis)
{
    float3 u = rotationAxis;
    float s = sin(angle); // sine of the angle
    float c = cos(angle);
    
    float3x3 identity = float3x3(
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1));
    
    float3x3 axisMatrix = float3x3(
    float3(0, -u.z, u.y),
    float3(u.z, 0, -u.x),
    float3(-u.y, u.x, 0));
    
    // https://math.stackexchange.com/questions/142821/matrix-for-rotation-around-a-vector
    float3x3 rotationMatrix = identity + (axisMatrix * s) + (mul(axisMatrix, axisMatrix) * ((1 - c) / pow(s, 2)));
    
    return normalize(mul(unitVector, rotationMatrix));
}

float getWindOffsetX(float2 pos)
{
    float offset = (sin((pos.x * frequency.x) + (time * speed.x)) * amplitude.x);
    return offset;
}

float getWindOffsetZ(float2 pos)
{
    float offset = (sin((pos.y * frequency.y) + (time * speed.y)) * amplitude.y);
    return offset;
}

[maxvertexcount(3)]
void main(point InputType input[1], inout TriangleStream<OutputType> triStream)
{
    OutputType output;
	
    float3 terrainNormal = input[0].normal;
    float3 defaultNormal = float3(0.0f, 0.0f, -1.0f);
    float grassBaseScale = 0.10f;
    float grassHeight = 1.5f;
    float4 vPos;
    
    float rotation = degrees(rand(input[0].tex));
    // rotate the normal so that it is perpendicular to the surface normal
    float3 newNormal = rotateUnitVector(defaultNormal, input[0].normal);
    // rotate the normal around the surface normal
    newNormal = rotateByAngle(newNormal, rotation, input[0].normal);
    
    float2 offset = float2(getWindOffsetX(input[0].position.xz), getWindOffsetZ(input[0].position.xz));
    // calculate the wind intesnsity for the X and Z wind
    offset.x = offset.x * abs(dot(newNormal, float3(-1, 0, 0)));
    offset.y = offset.y * abs(dot(newNormal, float3(0, 0, -1)));
    
    for (int x = 0; x < 3; x++)
    {
        // if this is one of two base vertices
        if (x != 2)
        {
            // rotate the base points so they're the tangent to the surface
            vPos = float4(rotateUnitVector(g_position[x], input[0].normal), 0.0f);
            // rotate the base points around the surface normal
            vPos = float4(rotateByAngle(vPos.xyz, rotation, input[0].normal), 0.0f);
            // scale the base points
            vPos *= grassBaseScale;
            // set the position of the base points
            vPos += input[0].position;
        }
        else
        {
            // set the height of the grass
            vPos = float4(input[0].normal * grassHeight, 0.0f);
            // set the default position
            vPos += input[0].position;
            // add the offset            
            vPos.xz += offset;
        }
        
        output.position = mul(vPos, worldMatrix);
        output.position = mul(output.position, viewMatrix);
        output.position = mul(output.position, projectionMatrix);
        output.depthPosition = output.position;
        triStream.Append(output);
        
    }
    
    triStream.RestartStrip();
}