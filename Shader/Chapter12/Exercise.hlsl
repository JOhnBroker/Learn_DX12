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
	float gPad0;
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
    float2 gPad1;
	     
	Light gLights[MaxLights];
};

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexPosHWNormalTex
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexPosHWNormalTex VS(VertexIn vin)
{
    VertexPosHWNormalTex vout;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;
    vout.PosH = mul(posW, gViewProj);
	
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, gMatTransform).xy;
	
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    
	return vout;
}

[maxvertexcount(6)]
void Cylinder_GS(line VertexPosHWNormalTex gin[2],
	inout TriangleStream<VertexPosHWNormalTex> triStream)
{
    // ***************************
    // 要求圆线是顺时针的，然后自底向上构造圆柱侧面           
    //   -->      v2____v3
    //  ______     |\   |
    // /      \    | \  |
    // \______/    |  \ |
    //   <--       |___\|
    //           v1(i1) v0(i0)
    // 固定长度

    float3 up = normalize(cross(gin[0].NormalW, (gin[1].PosW - gin[0].PosW)));
    VertexPosHWNormalTex v2, v3;
    
    v2.PosW = gin[1].PosW + up * 2.0f;
    v2.PosH = mul(float4(v2.PosW, 1.0f), gViewProj);
    v2.NormalW = gin[1].NormalW;
    v2.TexC = gin[1].TexC;
    
    v3.PosW = gin[0].PosW + up * 2.0f;
    v3.PosH = mul(float4(v3.PosW, 1.0f), gViewProj);
    v3.NormalW = gin[0].NormalW;
    v3.TexC = gin[0].TexC;
    
    triStream.Append(gin[0]);
    triStream.Append(gin[1]);
    triStream.Append(v2);
    triStream.RestartStrip();
    
    triStream.Append(v2);
    triStream.Append(v3);
    triStream.Append(gin[0]);
    triStream.RestartStrip();    
}

[maxvertexcount(2)]
void VertexNormal_GS(point VertexPosHWNormalTex gin[1],
	inout TriangleStream<VertexPosHWNormalTex> triStream)
{

}


[maxvertexcount(2)]
void PlaneNormal_GS(point VertexPosHWNormalTex gin[1],
	inout TriangleStream<VertexPosHWNormalTex> triStream)
{

}


float4 PS(VertexPosHWNormalTex pin) : SV_Target
{
    float4 diffuseColor = gDiffuseMap.Sample(gsamAnisotropicWrap, pin.TexC) * gDiffuseAlbedo;
	
#ifdef ALPHA_TEST
    clip(diffuseColor.a - 0.1f);
#endif
	
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
	
#ifdef FOG
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif
	
	litColor.a = diffuseColor.a;
    
	return litColor;
}
