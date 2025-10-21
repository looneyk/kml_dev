
cbuffer cbPerObject : register(b0)
{
	float4x4 m4World;
	float4x4 m4View;
	float4x4 m4Projection;
	float4 colorEmissive;
};

struct VertexIn
{
    float3 f3Position : POSITION;
    float3 f3Normal : NORMAL;
};

struct VertexOut
{
    float4 f4Position : SV_POSITION;
    float3 f3Normal : NORMAL;
};

VertexOut main(VertexIn vin)
{
    VertexOut vout;

    vout.f4Position = mul(float4(vin.f3Position, 1.f), m4World);
    vout.f4Position = mul(vout.f4Position, m4View);
    vout.f4Position = mul(vout.f4Position, m4Projection);
    vout.f3Normal = mul(float4(vin.f3Normal, 1.f), m4World).xyz;
    
    return vout;
}
