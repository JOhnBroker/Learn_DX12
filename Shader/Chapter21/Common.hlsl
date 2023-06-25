#include "LightingUtil.hlsl"

struct MaterialData
{
    float4		DiffuseAlbedo;
    float3		FresnelR0;
    float		Roughness;
    float4x4	MatTransform;
    float4x4	MatTransform1;
    uint		DiffuseMapIndex;
    uint		NormalMapIndex;
    uint		MatPad0;
    uint		MatPad1;
    float		Eta;
};

TextureCube gCubeMap : register(t0);
Texture2D gShadowMap : register(t1);
Texture2D gDiffuseMap[10] : register(t2);

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

SamplerState gSamPointWrap			: register(s0);
SamplerState gSamPointClamp			: register(s1);
SamplerState gSamLinearWrap			: register(s2);
SamplerState gSamLinearClamp		: register(s3);
SamplerState gSamAnisotropicWrap	: register(s4);
SamplerState gSamAnisotropicClamp	: register(s5);
SamplerComparisonState gSamShadow   : register(s6);

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
	float4x4 gView;
	float4x4 gInvView;
	float4x4 gProj;
	float4x4 gInvProj;
	float4x4 gViewProj;
	float4x4 gInvViewProj;
	float4x4 gInvSkyBoxWorld;
    float4x4 gShadowTransform;
    float4x4 gWorldInvTransposeView;
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

float3 BoxCubeMapLookup(float3 rayOrigin, float3 unitRayDir)
{
    float3 boxCenter = float3(0.0f, 0.0f, 0.0f);
    float3 boxExtents = float3(1.0f, 1.0f, 1.0f);
    float3 p = rayOrigin - boxCenter;
    
    // 参考资料https://cloud.tencent.com/developer/article/1055343
    float3 t1 = (-p + boxExtents) / unitRayDir;
    float3 t2 = (-p - boxExtents) / unitRayDir;

    float3 tmax = max(t1, t2);

    float t = min(min(tmax.x, tmax.y), tmax.z);

    return p + t * unitRayDir;
    
}

float3 NormalSampleToWorldSpace(float3 normalMapSample, 
	float3 unitNormalW, float3 tangentW)
{
	// [0,1] -> [-1,1]
    float3 normalT = 2.0f * normalMapSample - 1.0f;
	
    float3 N = unitNormalW;
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    float3 B = cross(N, T);
	
    float3x3 TBN = float3x3(T, B, N);
	
    float3 bumpedNormalW = mul(normalT, TBN);
    return bumpedNormalW;
}

float CalcShadowFactor(float4 shadowPosH)
{
    // 投影变换
    shadowPosH.xyz /= shadowPosH.w;
    
    float depth = shadowPosH.z;
    
    uint width, height, numMips;
    gShadowMap.GetDimensions(0, width, height, numMips);
    
    float dx = 1.0f / (float) width;
    
    float percentLit = 0.0f;
    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
        float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
        float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx),
    };
    
    [unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += gShadowMap.SampleCmpLevelZero(
        gSamShadow, shadowPosH.xy + offsets[i], depth).r;
    }
    return percentLit /= 9.0f;
}