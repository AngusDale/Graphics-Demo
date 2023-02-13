
Texture2D shaderTexture : register(t0);
Texture2D depthMapTexture[4][6] : register(t1);

SamplerState diffuseSampler : register(s0);
SamplerState shadowSampler : register(s1);

cbuffer LightBuffer : register(b0)
{
    float4 ambient[4];
    float4 diffuse[4];
    float4 position[4];
    float4 direction[4];
    float4 specularColour[4];
    float4 specularPower;
    
    int4 type;
    float4 outerAngle;
    float4 innerAngle;
    float4 falloff;
    float4 attenuation;
    float4 bias;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 viewVector : TEXCOORD1;    
    float3 worldPosition : TEXCOORD2;    
    float4 lightViewPos[4][6] : TEXCOORD3;
};

float4 calculateSpecular(float3 eyeVec, float3 lightVec, float3 normal, int index)
{
	// blinn-phong specular
    float3 halfway = normalize(eyeVec + lightVec);
    float NdotH = max(dot(halfway, normal), 0.0f);
    float specInten = pow(NdotH, specularPower[index]);
    return saturate(specInten * specularColour[index]);
}

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float calculateIntensity(float3 lightDirection, float3 normal)
{
    float intensity = saturate(dot(normal, lightDirection));
    return intensity;
}

float calculateAttenuation(float3 lightVec, float att)
{
    float dist = length(lightVec);
    float atten = 1.0f / (1.0f + (att * dist) + (0.0001f * pow(dist, 2)));
    return atten;
}

float4 calculateSpotlight(float3 lightVec, float3 norm, float4 texColor, float3 eyeVec, int index)
{
    float NdotL = saturate(dot(lightVec, norm));
    float4 finalColor = diffuse[index] * NdotL;
    
    // spotlight angle
    float spAngle = dot(lightVec, normalize(-direction[index].xyz));
    
    // cone attenuation
    float spot = saturate((spAngle - outerAngle[index]) / (innerAngle[index] - outerAngle[index]));
    // add fall off
    spot *= pow(spAngle, falloff[index]);
    
    float4 spec = spot * NdotL * calculateSpecular(eyeVec, lightVec, norm, index);
    
    return finalColor * spot + spec;
}

// Is the gemoetry in our shadow map
bool hasDepthData(float2 uv)
{
    if (uv.x < 0.f || uv.x > 1.f || uv.y < 0.f || uv.y > 1.f)
    {
        return false;
    }
    return true;
}

float isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    float depthMapSize = 2048.0f;
    float depthTexelSize = 1.0f / depthMapSize;
    int pcfSampleSize = 2; // sample a 5x5 area
    float pcfArea = pow(pcfSampleSize * 2 + 1, 2);
    float texelsNotInShadow = 0.0f;
    
    [unroll] 
    for (int x = -pcfSampleSize; x <= pcfSampleSize; x++)
    {
        [unroll] 
        for (int y = -pcfSampleSize; y <= pcfSampleSize; y++)
        {
            // Sample the shadow map (get depth of geometry)
            float2 newUV = uv + (float2(x, y) * depthTexelSize);
            float depthValue = sMap.Sample(shadowSampler, newUV).r;
            
	        // Calculate the depth from the light.
            float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
            lightDepthValue -= bias;
            // Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
            if (lightDepthValue < depthValue)
            {
                texelsNotInShadow += 1.0f;
            }
        }
    }
    
    texelsNotInShadow /= pcfArea;
    
    return texelsNotInShadow;
}

float2 getProjectiveCoords(float4 lightViewPosition)
{
    // Calculate the projected texture coordinates.
    float2 projTex = lightViewPosition.xy / lightViewPosition.w;
    projTex *= float2(0.5, -0.5);
    projTex += float2(0.5f, 0.5f);
    return projTex;
}

float4 main(InputType input) : SV_TARGET
{    
    float4 colour = float4(0.f, 0.f, 0.f, 1.f);
    float4 textureColour = shaderTexture.Sample(diffuseSampler, input.tex);   
    float4 lightColour = 0;
    float4 intensity, spec;
    float atten = 1.0f;
    float shadowFactor = 0.0f;
    
	//// Calculate the projected texture coordinates.
    float2 pTexCoord[4][6];
    for (int x = 0; x < 4; x++)
    {
        for (int y = 0; y < 6; y++)
        {
            pTexCoord[x][y] = getProjectiveCoords(input.lightViewPos[x][y]);
        }
    }
    
    // loop through all the lights
    for (int i = 0; i < 4; i++)
    {              
        shadowFactor = 0.0f;
        float3 lightVec = position[i].xyz - input.worldPosition;
        atten = calculateAttenuation(lightVec, attenuation[i]);
        lightVec = normalize(lightVec);
        
        switch (type[i])
        {
            // point light
            case 0:
                for (int j = 0; j < 6; j++)
                {
                    if (hasDepthData(pTexCoord[i][j]))
                    {
                        // Has depth map data               
                        shadowFactor = isInShadow(depthMapTexture[i][j], pTexCoord[i][j], input.lightViewPos[i][j], bias[i]);
                        if (shadowFactor)
                        {        
                            intensity = calculateIntensity(lightVec, input.normal) * shadowFactor;
                            spec = calculateSpecular(input.viewVector, lightVec, input.normal, i);
            
                            lightColour += ((intensity * diffuse[i]) + (intensity * spec)) * atten;
                            break; // only add one instance of lighting for the pointlight       
                        }
                    }
                }
                
                lightColour += ambient[i] * textureColour;
                break;
            
            // directional light
            case 1:
                if (hasDepthData(pTexCoord[i][0]))
                {
                    // Has depth map data
                    shadowFactor = isInShadow(depthMapTexture[i][0], pTexCoord[i][0], input.lightViewPos[i][0], bias[i]);
                    if (shadowFactor)
                    {
                        intensity = calculateIntensity(-direction[i].xyz, input.normal) * shadowFactor;
                        spec = calculateSpecular(input.viewVector, -direction[i].xyz, input.normal, i);
                        lightColour += (intensity * diffuse[i]) + (intensity * spec) * atten;
                    }
                }
                lightColour += ambient[i] * textureColour;
                break;
            
            // spot light
            case 2:
                if (hasDepthData(pTexCoord[i][0]))
                {
                    // Has depth map data
                    shadowFactor = isInShadow(depthMapTexture[i][0], pTexCoord[i][0], input.lightViewPos[i][0], bias[i]);
                
                    if (shadowFactor)
                    {
                        lightColour += shadowFactor * atten * calculateSpotlight(lightVec, normalize(input.normal), textureColour, normalize(input.viewVector), i);
                    }
                }
                lightColour += ambient[i] * textureColour;
                break;
        }

    }   
    
    return saturate(lightColour * textureColour);
}