#ifndef NUM_DIR_LIGHT
	#define NUM_DIR_LIGHT 1
#endif

#ifndef NUM_POINT_LIGHT
	#define NUM_POINT_LIGHT 0
#endif

#ifndef NUM_SPOT_LIGHT
	#define NUM_SPOT_LIGHT 0
#endif

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

struct VertexIn
{
	float3 PosL		:	POSITION;
};

struct VertexOut
{
	float3 PosL 	:	POSITION;
};

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

struct HullOut
{
    float3 PosL : POSITION;
};

struct DomainOut
{
    float4 PosH : SV_Position;
};

float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;
	
    return float4(invT * invT * invT,
		3.0f * t * invT * invT,
		3.0f * t * t * invT,
		t * t * t);
}

float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;
	
    return float4(-3.0f * invT * invT,
		3.0f * invT - 6.0f * t * invT,
		6.0f * t * invT - 3.0f * t * t,
		3.0f * t * t);
}

float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezpatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum = basisV.x * (basisU.x * bezpatch[0].PosL +
			basisU.y * bezpatch[1].PosL +
			basisU.z * bezpatch[2].PosL +
			basisU.w * bezpatch[3].PosL);

    sum += basisV.y * (basisU.x * bezpatch[4].PosL +
			basisU.y * bezpatch[5].PosL +
			basisU.z * bezpatch[6].PosL +
			basisU.w * bezpatch[7].PosL);
	
    sum += basisV.z * (basisU.x * bezpatch[8].PosL +
			basisU.y * bezpatch[9].PosL +
			basisU.z * bezpatch[10].PosL +
			basisU.w * bezpatch[11].PosL);
	
    sum += basisV.w * (basisU.x * bezpatch[12].PosL +
			basisU.y * bezpatch[13].PosL +
			basisU.z * bezpatch[14].PosL +
			basisU.w * bezpatch[15].PosL);
	
    return sum;
}

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
	return vout;
}

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
	
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

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p, uint i : SV_OutputControlPointID,
	uint patchID : SV_PrimitiveID)
{
    HullOut hout;
    hout.PosL = p[i].PosL;
    return hout;
}

[domain("quad")]
DomainOut DS(PatchTess patchTess, float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
	
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);
	
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
	
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	
    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}