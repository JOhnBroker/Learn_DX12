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

Texture2D gMainMap : register(t0);
Texture2D gAlphaMap : register(t1);

SamplerState gsamPointWrap			: register(s0);
SamplerState gsamPointClamp			: register(s1);
SamplerState gsamLinearWrap			: register(s2);
SamplerState gsamLinearClamp		: register(s3);
SamplerState gsamAnisotropicWrap	: register(s4);
SamplerState gsamAnisotropicClamp	: register(s5);
SamplerState gsamAnisotropicMulti	: register(s6);

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
	float gPad0;
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

struct VertexOut
{
	float4 PosH 	:	SV_POSITION;
	float3 PosW		:	POSITION;
	float3 NormalW	:	NORMAL;
    float2 TexC		:	TEXCOORD;
};

VertexOut VS (VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

	float4 posW = mul( float4(vin.PosL,1.0f), gWorld);
	vout.PosW = posW.xyz;
	vout.PosH = mul(posW,gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;
	
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);

	return vout;
}


float4 PS (VertexOut pin) : SV_Target
{
    float4 mainColor = gMainMap.Sample(gsamLinearWrap, pin.TexC);
    float4 alphaColor = gAlphaMap.Sample(gsamLinearWrap, pin.TexC);
    float4 diffuseColor = mainColor * alphaColor * gDiffuseAlbedo;
	
    pin.NormalW = normalize(pin.NormalW);
	
	float3 toEyeW = normalize(gEyePosW - pin.PosW);
    float4 ambient = gAmbientLight * diffuseColor;

	const float shininess = 1.0f - gRoughness;
    Material mat = { diffuseColor, gFresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, pin.NormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;
	litColor.a = diffuseColor.a;
    
	return litColor;
}
