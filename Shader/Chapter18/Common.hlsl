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
    float		Eta;
};

TextureCube gCubeMap : register(t0);
Texture2D gDiffuseMap[4] : register(t1);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

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
    uint gMaterialIndex;
    uint g_Pad0;
    uint g_Pad1;
    uint g_Pad2;
};

cbuffer cbPass : register(b1)
{
	float4x4 gInvSkyBoxWorld;
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
    //float3 gSkyBoxExtents;
    //float gPad2;
	
	Light gLights[MaxLights];
	
};
