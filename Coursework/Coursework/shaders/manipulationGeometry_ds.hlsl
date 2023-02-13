// Tessellation domain shader
// After tessellation the domain shader processes the all the vertices

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    matrix lightViewMatrix[4][6];
    matrix lightProjectionMatrix[4];
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
}

cbuffer CameraBuffer : register(b2)
{
    float3 cameraPosition;
    float pad3;
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
};

// calculate the height map offset
float getHeight(float2 uv)
{
    float offset = texture0.SampleLevel(sampler0, uv, 0).r;
    return offset * height;
}

// calculate the wave offset
float getWaveOffset(float2 pos)
{
    float offset = ((sin((pos.x * frequency.x) + (time * speed.x)) * amplitude.x) + (sin((pos.y * frequency.y) + (time * speed.y)) * amplitude.y)) / 2.0f;
    return offset;
}

// calculate the wave normal using the NESW method
float3 calculateWaveNormal(float3 oldNormal, float3 position, float amp)
{
    float WorldStep = 1.0f / 5.0f;
    
    float hN = getWaveOffset(float2(position.x, position.z + WorldStep));
    float hS = getWaveOffset(float2(position.x, position.z - WorldStep));
    float hE = getWaveOffset(float2(position.x + WorldStep, position.z));
    float hW = getWaveOffset(float2(position.x - WorldStep, position.z));
    
    float h = getWaveOffset(position.xz);
    
    float3 tan1 = normalize(float3(WorldStep, hE - h, 0.0f));
    float3 tan2 = normalize(float3(-WorldStep, hW - h, 0.0f));
    float3 bi1 = normalize(float3(0.0f, hN - h, WorldStep));
    float3 bi2 = normalize(float3(0.0f, hS - h, -WorldStep));
    
    float3 n1 = cross(bi1, tan1);
    float3 n2 = cross(tan1, bi2);
    float3 n3 = cross(bi2, tan2);
    float3 n4 = cross(tan2, bi1);
    return (n1 + n2 + n3 + n4) * 0.25f;
}

// calculate the heightmap normal
float3 calculateNormal(float2 uv)
{
    float u = (1.0f / 150.0f);
    float WorldStep = 1.0f/5.0f;
    
    float hN = getHeight(float2(uv.x, uv.y + u));
    float hS = getHeight(float2(uv.x, uv.y - u));
    float hE = getHeight(float2(uv.x + u, uv.y));
    float hW = getHeight(float2(uv.x - u, uv.y));
    
    float h = getHeight(uv);
    
    float3 tan1 = normalize(float3(WorldStep, hE - h, 0.0f));
    float3 tan2 = normalize(float3(-WorldStep, hW - h, 0.0f));
    float3 bi1 = normalize(float3(0.0f, hN - h, WorldStep));
    float3 bi2 = normalize(float3(0.0f, hS - h, -WorldStep));
    
    float3 n1 = cross(bi1, tan1);
    float3 n2 = cross(tan1, bi2);
    float3 n3 = cross(bi2, tan2);
    float3 n4 = cross(tan2, bi1);
    return (n1 + n2 + n3 + n4) * 0.25f;
}

// rotate the normal onto the sphere
float3 rotateNormal(float3 flatNorm, float3 newSurfaceNormal)
{
    // set the up vector
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 b = normalize(newSurfaceNormal); // to be rotated to vector b
    float3 u = normalize(cross(newSurfaceNormal, up)); // axis of rotation   
    
    float c = dot(up, b); // cosine of the angle
    float angle = acos(c);
    float s = sin(angle); // sine of the angle
    
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
    
    return normalize(mul(flatNorm, rotationMatrix));
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
    
    // set the radius to the vertex
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
    
    float3 n1 = lerp(patch[0].normal, patch[1].normal, uvwCoord.y);
    float3 n2 = lerp(patch[3].normal, patch[2].normal, uvwCoord.y);
    normal = lerp(n1, n2, uvwCoord.x);
    
    // -- VERTEX MANIPULATION
    float3 center = spherePos.xyz;
	// calculate height of vertex
    float4 textureColour = texture0.SampleLevel(sampler0, texCoord, 0);
    // calculate wave offset 
    float offset = getWaveOffset(vertexPosition.xz);
    // adjust the position by the texture and offset
    vertexPosition.y = height * textureColour.r + offset;
    // calculate the wave normals before adjusting the position
    float3 waveNormal = calculateWaveNormal(normal, vertexPosition, 1.0f);
    // get the position of the vertex, lerp'd betweek the flat and spherical plane
    vertexPosition = lerp(vertexPosition, pointToSphere(vertexPosition, texCoord, center), planeToSphere);
    
    // calculate the normal based on the height map    
    float3 heightMapNormal = calculateNormal(texCoord);
    float3 flatNormal = (waveNormal + heightMapNormal) / 2.0f;
    // rotate that normal onto the sphere
    float3 normalOnSphere = rotateNormal(flatNormal, vertexPosition - center);
    normal = lerp(flatNormal, normalOnSphere, planeToSphere);
    
    // Calculate the position of the new vertex against the world, view, and projection matrices.
    output.position = float4(vertexPosition, 1.0f);
    
    // Calculate the normal vector against the world matrix only and normalise.
    output.normal = normal;
    
    output.tex = texCoord;

    return output;
}

