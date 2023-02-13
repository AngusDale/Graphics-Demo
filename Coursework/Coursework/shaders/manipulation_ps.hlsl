// Light pixel shader
// Calculate diffuse lighting for a single directional light (also texturing)

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
    float4 shadowMapBias;
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
    float atten = 1.0f / (1.0f + (att * dist) + (0.0f * pow(dist, 2)));
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
    
    return saturate(finalColor * spot + spec);
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

bool isInShadow(Texture2D sMap, float2 uv, float4 lightViewPosition, float bias)
{
    // Sample the shadow map (get depth of geometry)
    float depthValue = sMap.Sample(shadowSampler, uv).r;
	// Calculate the depth from the light.
    float lightDepthValue = lightViewPosition.z / lightViewPosition.w;
    lightDepthValue -= bias;

	// Compare the depth of the shadow map value and the depth of the light to determine whether to shadow or to light this pixel.
    if (lightDepthValue < depthValue)
    {
        return false;
    }
    return true;
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
    float shadowBias = 0.005f;
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
        float3 lightVec = position[i].xyz - input.worldPosition;
        atten = calculateAttenuation(lightVec, attenuation[i]);
        lightVec = normalize(lightVec);
        
        // excecute based on which light type it is
        switch (type[i])
        {
            // point light
            case 0:
                for (int j = 0; j < 6; j++)
                {
                    if (hasDepthData(pTexCoord[i][j]))
                    {
                        // Has depth map data
                        if (!isInShadow(depthMapTexture[i][j], pTexCoord[i][j], input.lightViewPos[i][j], shadowMapBias[i]))
                        {
                            intensity = calculateIntensity(lightVec, input.normal);
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
                    if (!isInShadow(depthMapTexture[i][0], pTexCoord[i][0], input.lightViewPos[i][0], shadowMapBias[i]))
                    {
                        intensity = calculateIntensity(-direction[i].xyz, input.normal);
                        spec = calculateSpecular(input.viewVector, -direction[i].xyz, input.normal, i);
                        lightColour += (intensity * diffuse[i]) + (intensity * spec) * atten;
                    }
                }
                lightColour += ambient[i] * textureColour;
                break;
            
            // spot light
            case 2:
                shadowBias = 0.01f;
                if (hasDepthData(pTexCoord[i][0]))
                {
                    // Has depth map data
                    if (!isInShadow(depthMapTexture[i][0], pTexCoord[i][0], input.lightViewPos[i][0], shadowMapBias[i]))
                    {
                        lightColour += atten * calculateSpotlight(lightVec, normalize(input.normal), textureColour, normalize(input.viewVector), i);
                    }
                }
                lightColour += ambient[i] * textureColour;
                break;
            }

        }
    
    return saturate(lightColour * textureColour);
    
    ////input.normal = calculateNormal(input.tex);
    //float4 colour = 0;
    //float4 green = float4(0.0f, 0.3f, 0.1f, 1.0f);
    //float4 brown = float4(0.6f, 0.3f, 0.0f, 1.0f);
    //float4 white = float4(0.7, 0.7, 0.8, 1);
       
    //// interpolate the colour based on the y position of the vertex
    //float heightInter = smoothstep(3.0f, 7.0f, input.worldPosition.y);
    //colour = lerp(brown, green, heightInter);
    
    //// if in range of the lower bound of the green - white interpolation... interpolate
    //if (input.worldPosition.y > 20.0f)
    //{
    //    heightInter = smoothstep(20.0f, 23.0f, input.worldPosition.y);
    //    colour = lerp(green, white, heightInter);
    //}
    
    //float angleInter = 0.0f;
    //// check the steepness of the vertex, interpolate between the previously calculated colour and brown
    //if (input.normal.y < 0.55f)
    //{
    //    angleInter = smoothstep(0.55f, 0.4f, input.normal.y);
    //    colour = lerp(colour, brown, angleInter);
    //}
        
    //// calculate lighting
    //lightColour = float4(0.1f, 0.27f, 0.37f, 1.0f) + calculateLighting(lightVec, input.normal, colour);
    
    //// add specular if the lowest value out of the colour's RGB is above 0.1. In this case blue will only ever be greater than 0.1 if we are interpolating to white.
    //if (colour.b > 0.1)
    //{
    //    float specIntensity = (heightInter + angleInter) / 2;
    //    lightColour += calculateSpecular(input.viewVector, lightVec, input.normal) * specIntensity;
    //}
    
    //return normalize(calculateLighting(float3(-0.5, 0.5, 0), input.normal, diffuseColour) * textureColour + ambient * textureColour);
    //return lightColour;
}



