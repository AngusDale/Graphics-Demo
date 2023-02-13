// Tessellation vertex shader.
// Doesn't do much, could manipulate the control points
// Pass forward data, strip out some values not required for example.

struct InputType
{
    float3 position : POSITION;
    float3 normal : NORMAL;    
    float2 tex : TEXCOORD0;
};

struct OutputType
{
    float3 position : POSITION;
    float3 normal : NORMAL;    
    float2 tex : TEXCOORD0;
};

OutputType main(InputType input)
{
    OutputType output;

	 // Pass the vertex position into the hull shader.
    output.position = input.position;
    // pass the normal to the hull shader
    output.normal = input.normal;
    // Pass the texture coords into the hull shader.
    output.tex = input.tex;
    
    return output;
}
