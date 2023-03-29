#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap	: register(s4);
SamplerState gsamAnisotropicClamp	: register(s5);

// constant buffer

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;	
    float4x4 gTexTransform;
};

cbuffer cbMaterial : register(b1)
{
	float4 gDiffuseAlbedo;
	float3 gFresnelR0;
	float  gRoughness;
	float4x4 gMatTransform;   
};

cbuffer cbPass : register(b2)
{
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float3 gEyePosW;
	float gPad1;
	float2 gRenderTargetSize;
	float2 gInvRenderTargetSize;
	float gNearZ;
	float gFarZ;
	float gTotalTime;
	float gDeltaTime;
	float4 gAmbientLight;

	// fog
    float4 gFogColor;
    float gFogStart;
    float gFogRange;
    float2 gPad2;
	
	Light gLights[MaxLights];
};

struct Basic_VertexIn
{
	float3 PosL		:	POSITION;
};

struct Basic_VertexOut
{
	float3 PosL 	:	POSITION;
};

struct Sphere_VertexIn
{
    float3 PosL     : POSITION;
    float3 Normal   : NORMAL;
    float2 TexC     : TEXCOORD;
};

// Quad

Basic_VertexOut Basic_VS(Basic_VertexIn vin)
{
    Basic_VertexOut vout;
    vout.PosL = vin.PosL;
	return vout;
}

struct Basic_PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

Basic_PatchTess Basic_ConstantHS(InputPatch<Basic_VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    Basic_PatchTess pt;
	
    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + 
		patch[2].PosL + patch[3].PosL);
    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
	
    float d = distance(centerW, gEyePosW);
	
    const float d0 = 20.0f;
    const float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
	
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;
	
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
	
    return pt;
}

struct Basic_HullOut
{
    float3 PosL : POSITION;
};

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("Basic_ConstantHS")]
[maxtessfactor(64.0f)]
Basic_HullOut Basic_HS(InputPatch<Basic_VertexOut, 4> p, uint i : SV_OutputControlPointID,
	uint patchID : SV_PrimitiveID)
{
    Basic_HullOut hout;
    hout.PosL = p[i].PosL;
    return hout;
}

struct Basic_DomainOut
{
    float4 PosH : SV_Position;
};

[domain("quad")]
Basic_DomainOut Basic_DS(Basic_PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<Basic_HullOut, 4> quad)
{
    Basic_DomainOut dout;
	
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);
	
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
	
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	
    return dout;
}

float4 Basic_PS(Basic_DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}


// Sphere

Sphere_VertexIn Sphere_VS(Sphere_VertexIn vin)
{
    Sphere_VertexIn vout;
    vout.PosL = vin.PosL;
    vout.Normal = vin.PosL;
    vout.TexC = vout.TexC;
    return vout;    
}

struct Sphere_PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess[1] : SV_InsideTessFactor;
};

Sphere_PatchTess Sphere_ConstantHS(InputPatch<Sphere_VertexIn, 3> patch,
    uint patchID : SV_PrimitiveID)
{
    Sphere_PatchTess pt;
    
    float3 centerL = 0.33f * (patch[0].PosL +
        patch[1].PosL + patch[2].PosL);
    float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;
    
    float d = distance(centerW, gEyePosW);
    
    const float d0 = 20.0f;
    const float d1 = 100.0f;
    float tess = 64.0f * saturate((d1 - d) / (d1 - d0));
    
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
    
    return pt;
}

struct Sphere_HullOut
{
    float3 PosL : POSITION;
    float3 Normal : NORMAL;
    float2 TexC : TEXCOORD;
};

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("Sphere_ConstantHS")]
[maxtessfactor(64.0f)]
Sphere_HullOut Sphere_HS(InputPatch<Sphere_VertexIn, 3> patch,
    uint id : SV_OutputControlPointID,
    uint patchID : SV_PrimitiveID)
{
    Sphere_HullOut hout;
    hout.PosL = patch[id].PosL;
    hout.Normal = patch[id].Normal;
    hout.TexC = patch[id].TexC;
    return hout;
}

struct Sphere_DomainOut
{
    float3 PosW     : POSITION;
    float4 PosH     : SV_Position;
    float3 NormalW  : NORMAL;
    float2 TexC     : TEXCOORD;
};

[domain("tri")]
Sphere_DomainOut Sphere_DS(Sphere_PatchTess patchTess,
    float3 uvw : SV_DomainLocation, 
    const OutputPatch<Sphere_HullOut, 3> tri)
{
    Sphere_DomainOut dout;
    
    float3 pos = normalize(tri[0].PosL * uvw.x + tri[1].PosL * uvw.y + tri[2].PosL * uvw.z);
    float3 normal = tri[0].Normal * uvw.x + tri[1].Normal * uvw.y + tri[2].Normal * uvw.z;
    float2 texC = tri[0].TexC * uvw.x + tri[1].TexC * uvw.y + tri[2].TexC * uvw.z;
    
    float4 posW = mul(float4(pos, 1.0f), gWorld);
    dout.PosW = pos;
    dout.PosH = mul(posW, gViewProj);
    dout.NormalW = normalize(mul(normal, (float3x3) gWorld));
    dout.TexC = mul(float4(texC, 0.0f, 1.0f), gMatTransform).xy;
    
    return dout;
}

float4 Sphere_PS(Sphere_DomainOut pin) : SV_Target
{
    float4 diffuseColor = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
    pin.NormalW = normalize(pin.NormalW);
	
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;
    float4 ambient = gAmbientLight * diffuseColor;

    const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseColor, gFresnelR0, shininess };
    float3 shadowFactor = 1.0f;
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

    float4 litColor = ambient + directLight;
	
    litColor.a = diffuseColor.a;
    
    return litColor;
}