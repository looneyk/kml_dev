//------------------------------------------------------------------------------
//
//  Copyright (c) 2013 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  color.fx
//
//------------------------------------------------------------------------------

uniform extern float4x4 gWVP;

struct OutputVS
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

OutputVS ColorVS(float3 posL : POSITION0, float4 c : COLOR0)
{
    OutputVS outVS = (OutputVS)0;
    outVS.position = mul(float4(posL, 1.0f), gWVP); // Transform to homogeneous clip space.
    outVS.color = c; // Just pass the vertex color into the pixel shader.

    return outVS;
}

float4 ColorPS(float4 c : COLOR0) : COLOR
{
    return c;
}

technique ColorTech
{
    pass P0
    {
        vertexShader = compile vs_2_0 ColorVS();
        pixelShader  = compile ps_2_0 ColorPS();
    }
}
