
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
    float padding;
    
    float2 amplitude;
    float2 frequency;
    float2 speed;
    float2 pad;
    
    float4 spherePos;
}

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

float3 calculateWaveNormal(float3 oldNormal, float3 position, float amp)
{
    float WorldStep = 1.0f;
    
    float hN = getWaveOffset(float2(position.x,             position.z + WorldStep));
    float hS = getWaveOffset(float2(position.x,             position.z - WorldStep));
    float hE = getWaveOffset(float2(position.x + WorldStep, position.z            ));
    float hW = getWaveOffset(float2(position.x - WorldStep, position.z            ));
    
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

float3 calculateNormal(float2 uv)
{
    float u = (1.0f / 100.0f);
    
    float WorldStep = 100.0f * u;
    
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

float3 rotateNormal(float3 flatNorm, float3 newSurfaceNormal)
{
    float3 up = float3(0.0f, 1.0f, 0.0f);
    
    float3 a = normalize(up);                           // vector a
    float3 b = normalize(newSurfaceNormal);             // to be rotated to vector b
    float3 u = normalize(cross(newSurfaceNormal, up)); // axis of rotation   
    
    float c = dot(a, b); // cosine of the angle
    float angle = acos(c);
    float s = sin(angle); // sine of the angle
    
    float3x3 identity = float3x3(
    float3(1, 0, 0),
    float3(0, 1, 0),
    float3(0, 0, 1));
    
    float3x3 axisMatrix = float3x3(
    float3(   0, -u.z,  u.y),
    float3( u.z,    0, -u.x),
    float3(-u.y,  u.x,    0));
    
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
    float resolution = 1 / 100.0f;
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

OutputType main(InputType input)
{
    OutputType output;
    
    float3 center = spherePos.xyz;
    
	// calculate height of vertex
    float4 textureColour = texture0.SampleLevel(sampler0, input.tex, 0);   
    // calculate wave offset 
    float offset = getWaveOffset(input.position.xz);
    // adjust the position by the texture and offset
    input.position.y = height * textureColour.r + offset;
    
    // calculate the wave normals before adjusting the position
    float3 waveNormal = calculateWaveNormal(input.normal, input.position.xyz, 1.0f);
    
    // get the position of the vertex, lerp'd betweek the flat and spherical plane
    input.position = lerp(input.position, float4(pointToSphere(input.position.xyz, input.tex, center), 1.0f), planeToSphere);
    
    // calculate the normal based on the height map    
    float3 heightMapNormal = calculateNormal(input.tex);
    float3 flatNormal = (waveNormal + heightMapNormal) / 2.0f;
    // rotate that normal onto the sphere
    float3 normalOnSphere = rotateNormal(flatNormal, input.position.xyz - center);
    input.normal = lerp(flatNormal, normalOnSphere, planeToSphere);
    
	// Calculate the position of the vertex against the world, view, and projection matrices.
    output.position = mul(input.position, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);
    
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 6; j++)
        {
            output.lightViewPos[i][j] = mul(input.position, worldMatrix);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightViewMatrix[i][j]);
            output.lightViewPos[i][j] = mul(output.lightViewPos[i][j], lightProjectionMatrix[i]);
        }
    }

	// Store the texture coordinates for the pixel shader.
    output.tex = input.tex;

    output.worldPosition = mul(input.position, worldMatrix).xyz;
	
	// Calculate the normal vector against the world matrix only and normalise.
    output.normal = mul(input.normal, (float3x3) worldMatrix);
    
    output.viewVector = normalize(cameraPosition.xyz - output.worldPosition.xyz);
    return output;
}