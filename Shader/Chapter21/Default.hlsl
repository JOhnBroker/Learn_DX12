#ifndef NUM_DIR_LIGHT
	#define NUM_DIR_LIGHT 0
#endif

#ifndef NUM_POINT_LIGHT
	#define NUM_POINT_LIGHT 0
#endif

#ifndef NUM_SPOT_LIGHT
	#define NUM_SPOT_LIGHT 0
#endif

#include "Common.hlsl"

struct VertexIn
{
	float3 PosL		:	POSITION;
	float3 NormalL	:	NORMAL;
	float2 TexC		:	TEXCOORD;
    float3 TangentU :   TANGENT;
};

struct VertexOut
{
	float4 PosH 	    :	SV_POSITION;
    float4 ShadowPosH   :   POSITION0;
    float4 SsaoPosH     :   POSITION1;
	float3 PosW		    :	POSITION2;
	float3 NormalW	    :	NORMAL;
    float2 TexC		    :	TEXCOORD;
    float3 TangentW     :   TANGENT;
};

struct SkyVertexOut
{
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    MaterialData matData = gMaterialData[gMaterialIndex];
	
	float4 posW = mul( float4(vin.PosL,1.0f), gWorld);
	vout.PosW = posW.xyz;
	
    vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    
    vout.PosH = mul(posW, gViewProj);
    vout.SsaoPosH = mul(posW, gViewProjTex);
    vout.ShadowPosH = mul(posW, gShadowTransform);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
    
	return vout;
}

float4 PS (VertexOut pin) : SV_Target
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    uint normalIndex = matData.NormalMapIndex;
	
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gSamAnisotropicWrap, pin.TexC);
	
#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    pin.NormalW = normalize(pin.NormalW);
    float4 normalMapSample = gDiffuseMap[normalIndex].Sample(gSamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.xyz, pin.NormalW, pin.TangentW);
    
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

    float ao = 1.0f;
#ifdef SSAO_ENABLE
    pin.SsaoPosH /= pin.SsaoPosH.w;
    ao = gSSAOMap.Sample(gSamLinearClamp, pin.SsaoPosH.xy, 0.0f).r;
#endif

    float4 ambient = ao * gAmbientLight * diffuseAlbedo;
    float3 shadowFactor = float3(1.0f, 1.0f, 1.0f);
    
#ifdef SHADOW_ENABLE
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);
#endif

    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, bumpedNormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;

    // slab method
    //float4 rayOrigin = mul(float4(pin.PosW, 1.0f), gInvSkyBoxWorld);
    //float3 rayDir = reflect(-toEyeW, pin.NormalW);
    //float3 r = BoxCubeMapLookup(rayOrigin.xyz, normalize(rayDir));
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gSamLinearWrap, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

	litColor.a = diffuseAlbedo.a;
    
	return litColor;
}

SkyVertexOut Sky_VS(VertexIn vin)
{
    SkyVertexOut vout;
    
    vout.PosL = vin.PosL;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    
    // Always center sky about camera.
    posW.xyz += gEyePosW;
    
    // Set z = w so that z/w = 1 (i.e., skydome always on far plane).
    vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

float4 Sky_PS(SkyVertexOut pin) : SV_Target
{
    return gCubeMap.Sample(gSamLinearWrap, pin.PosL);
}


