#ifndef NUM_DIR_LIGHT
	#define NUM_DIR_LIGHT 2
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
	float4 PosH 	:	SV_POSITION;
	float3 PosW		:	POSITION;
	float3 NormalW	:	NORMAL;
    float2 TexC		:	TEXCOORD;
    float3 TangentW :   TANGENT;
};

struct SkyVertexOut
{
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
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

VertexOut VS(VertexIn vin)
{
	VertexOut vout = (VertexOut)0.0f;

    MaterialData matData = gMaterialData[gMaterialIndex];
	
	float4 posW = mul( float4(vin.PosL,1.0f), gWorld);
	vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MatTransform).xy;
	
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    
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
    float eta = 1.0f / matData.Eta;
	
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gsamLinearWrap, pin.TexC);
	
    pin.NormalW = normalize(pin.NormalW);
    float4 normalMapSample = gDiffuseMap[normalIndex].Sample(gsamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.xyz, pin.NormalW, pin.TangentW);
	
    float3 toEyeW = normalize(gEyePosW - pin.PosW);
    float4 ambient = gAmbientLight * diffuseAlbedo;

    const float shininess = (1.0f - roughness) * normalMapSample.a;
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
	float3 shadowFactor = 1.0f;
	float4 directLight = ComputeLighting(gLights, mat, pin.PosW, bumpedNormalW, toEyeW, shadowFactor);

	float4 litColor = ambient + directLight;
	
#ifndef REFRACT 
    // slab method
    //float4 rayOrigin = mul(float4(pin.PosW, 1.0f), gInvSkyBoxWorld);
    //float3 rayDir = reflect(-toEyeW, pin.NormalW);
    //float3 r = BoxCubeMapLookup(rayOrigin.xyz, normalize(rayDir));
    float3 r = reflect(-toEyeW, bumpedNormalW);
    float4 reflectionColor = gCubeMap.Sample(gsamLinearClamp, r);
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
#else
    if(eta < 1.0f)
    {
        float3 refractColor = float3(0.8f, 0.8f, 0.8f);
        float3 r = refract(-toEyeW, bumpedNormalW, eta);
        float4 refractionColor = gCubeMap.Sample(gsamLinearWrap, r);
        litColor.rgb += refractColor * refractionColor.rgb;
    }
#endif

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
    return gCubeMap.Sample(gsamLinearWrap, pin.PosL);

}