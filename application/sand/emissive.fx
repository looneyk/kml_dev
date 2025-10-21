//------------------------------------------------------------------------------
//
//  Copyright (c) 2010 Keith Michael Looney. All Rights Reserved.
//
//------------------------------------------------------------------------------
//
//  emissive.fx
//
//------------------------------------------------------------------------------

uniform extern float4x4 gWVP;
uniform extern float4 gEmissiveMtrl;

struct OutputVS
{
    float4 position : POSITION0;
    float4 color : COLOR0;
};

OutputVS EmissiveVertexShader(float3 posL : POSITION0)
{
    OutputVS outVS = (OutputVS)0;
    outVS.position = mul(float4(posL, 1.0f), gWVP); // Transform to homogeneous clip space.
    outVS.color = gEmissiveMtrl;

    return outVS;
}

float4 EmissivePixelShader(float4 c : COLOR0) : COLOR
{
    return c;
}

technique EmissiveLightingTechnique
{
    pass P0
    {
        vertexShader = compile vs_2_0 EmissiveVertexShader();
        pixelShader  = compile ps_2_0 EmissivePixelShader();
    }
}



//struct Material
//{
//    float4 Ka;
//    float4 Ke;
//    float4 Kd;
//};
//
//float4 lighting(Material material, Light light, float4 globalAmbient, float4 position, float3 normal, float3 eyePosition)
//{
//    float4 emissive = material.Ke;
//
//    return emissive;
//}
