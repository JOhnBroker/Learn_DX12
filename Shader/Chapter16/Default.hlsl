#ifndef NUM_DIR_LIGHT
	#define NUM_DIR_LIGHT 3
#endif

#ifndef NUM_POINT_LIGHT
	#define NUM_POINT_LIGHT 0
#endif

#ifndef NUM_SPOT_LIGHT
	#define NUM_SPOT_LIGHT 0
#endif

#include "LightingUtil.hlsl"

struct InstanceData
{
    float4x4	World;
    float4x4	TexTransform;
    uint		MaterialIndex;
    uint		ObjPad0;
    uint		ObjPad1;
    uint		ObjPad2;
};

struct MaterialData
{
    float4		DiffuseAlbedo;
    float3		FresnelR0;
    float		Roughness;
    float4x4	MatTransform;
    uint		DiffuseMapIndex;
    uint		MatPad0;
    uint		MatPad1;
    uint		MatPad2;
};

Texture2D gDiffuseMap[5] : register(t0);
StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t1, space1);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap	: register(s4);
SamplerState gsamAnisotropicClamp	: register(s5);

// constant buffer

cbuffer cbPass : register(b0)
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
	
	Light gLights[MaxLights];
};

struct VertexIn
{
	float3 PosL		:	POSITION;
	float3 NormalL	:	NORMAL;
	float2 TexC		:	TEXCOORD;
};

struct InstanceIn
{
    float3 PosL			: POSITION;
    float3 NormalL		: NORMAL;
    float2 TexC			: TEXCOORD;
    float4x4 World		: WORLD;
    float4x4 WorldInvTranspose : WorldInvTranspose;
    uint MatIndex		: MATINDEX;
};

struct VertexOut
{
	float4 PosH 	:	SV_POSITION;
	float3 PosW		:	POSITION;
	float3 NormalW	:	NORMAL;
    float2 TexC		:	TEXCOORD;
	// nointerpolation is used so the index is not interpolated across the triangle.
    nointerpolation uint MatIndex : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout = (VertexOut)0.0f;

    InstanceData instData = gInstanceData[instanceID];
    float4x4 world = instData.World;
    float4x4 texTransform = instData.TexTransform;
    uint matIndex = instData.MaterialIndex;
	
    vout.MatIndex = matIndex;
	
    MaterialData matData = gMaterialData[matIndex];
	
	float4 posW = mul( float4(vin.PosL,1.0f), world);
	vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), texTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
	vout.NormalW = mul(vin.NormalL, (float3x3)world);

	return vout;
}


float4 PS (VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[pin.MatIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
	
    float4 diffuseColor = gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC) * diffuseAlbedo;
	
#ifdef ALPHA_TEST
    clip(diffuseColor.a - 0.1f);
#endif
	
    pin.NormalW = normalize(pin.NormalW);
	
	float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW);
    toEyeW /= distToEye;
    float4 ambient = gAmbientLight * diffuseColor;

    const float shininess = 1.0f - roughness;
    Material mat = { diffuseColor, fresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;
	
#ifdef FOG
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif
	
	litColor.a = diffuseColor.a;
    
	return litColor;
}
