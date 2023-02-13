// Light pixel shader
// Calculate diffuse lighting for a single directional light (also texturing)

Texture2D texture0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer LightBuffer : register(b0)
{
    float4 ambient[6];
    float4 diffuse[6];
    float4 position[6];
    float4 direction[6];
        
    float type[6];
    float2 padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 worldPosition : TEXCOORD1;
    float3 viewVector : TEXCOORD2;
};

float calculateAttenuation(float3 lightVec, float3 att)
{
    float dist = length(lightVec);
    float atten = 1 / (att.x + (att.y * dist) + (att.z * pow(dist, 2)));
    return atten;
}

float4 calculateSpecular(float3 eyeVec, float3 lightVec, float3 normal, float4 colour, float power)
{
	// blinn-phong specular
    float3 halfway = normalize(eyeVec + lightVec);
    float NdotH = max(dot(halfway, normal), 0.0f);
    float specInten = pow(NdotH, power);
    return saturate(specInten * colour);
}

// Calculate lighting intensity based on direction and normal. Combine with light colour.
float4 calculateLighting(float3 lightDirection, float3 normal, float4 ldiffuse)
{
    float intensity = saturate(dot(normal, lightDirection));
    float4 colour = saturate(ldiffuse * intensity);
    return colour;
}

//float4 calculateSpotlight(float3 lightVec, float3 norm, float4 texColor, float3 eyeVec, float4 diffuseColour, float3 lightDirection)
//{
//    float distToLight = length(lightVec);
    
//    // diffuse
//    // normalize lightVector
//    lightVec /= distToLight;
//    float NdotL = saturate(dot(lightVec, norm));
//    float4 finalColor = diffuseColour * NdotL * texColor;
//    // spotlight angle
//    float spAngle = dot(lightVec, normalize(-lightDirection));
//    // cone attenuation
//    float spot = saturate((spAngle - outerAngle) / (innerAngle - outerAngle));
//    // add fall off
//    spot *= pow(spAngle, falloff);
   
//    // specular
//    float3 halfway = normalize(eyeVec + lightVec);
//    float NdotH = saturate(dot(halfway, norm));
//    float specInten = pow(NdotH, specularPower) * specularOpacity;
    
//    return saturate(finalColor * spot) + (ambientColour * texColor) + specInten;
//}

float4 main(InputType input) : SV_TARGET
{
	// Sample the texture. Calculate light intensity and colour, return light*texture for final pixel colour.
    float4 textureColour = texture0.Sample(sampler0, input.tex);
    float4 finalColour = 0.0f;

    for (int i = 0; i < 3; i++)
    {
        float3 lightVector;
        float atten = 1.0f;
        
        if (type[i] == 0 )
        {
            //lightVector = normalize(position - input.worldPosition);
            //atten = calculateAttenuation(lightVector);
        }
        else if (type[i] == 1)
        {
            //lightVector = -direction;
        }

        float4 lightColour = calculateLighting(lightVector, input.normal, diffuse[i]) * atten;
        //finalColour += lightColour + calculateSpecular(input.viewVector, lightVector, input.normal, specularColour, specularPower);
    }

    //finalColour += ambient; 
    
    return finalColour * textureColour;
}



