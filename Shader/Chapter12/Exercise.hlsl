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

VertexIn Sphere_VS(VertexIn vin)
{
    VertexIn vout;
    
    vout.PosL = vin.PosL;
    vout.NormalL = vin.NormalL;
    vout.TexC = vin.TexC;
    
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

void Subdivide(VertexIn inVerts[3], out VertexPosHWNormalTex outVerts[6])
{
     //       v1
    //       *
    //      / \
	//     /   \
	//  m0*-----*m1
    //   / \   / \
	//  /   \ /   \
	// *-----*-----*
    // v0    m2     v2
    
    VertexPosHWNormalTex m[3];

    float4 posW0 = mul(float4((0.5f * (inVerts[0].PosL + inVerts[1].PosL)), 1.0f), gWorld);
    float4 posW1 = mul(float4((0.5f * (inVerts[1].PosL + inVerts[2].PosL)), 1.0f), gWorld);
    float4 posW2 = mul(float4((0.5f * (inVerts[0].PosL + inVerts[2].PosL)), 1.0f), gWorld);
    
    m[0].PosW = normalize(posW0.xyz);
    m[1].PosW = normalize(posW1.xyz);
    m[2].PosW = normalize(posW2.xyz);
    m[0].PosH = mul(float4(m[0].PosW, 1.0f), gViewProj);
    m[1].PosH = mul(float4(m[1].PosW, 1.0f), gViewProj);
    m[2].PosH = mul(float4(m[2].PosW, 1.0f), gViewProj);
    
    m[0].NormalW = normalize(mul(0.5f * (inVerts[0].NormalL + inVerts[1].NormalL), (float3x3) gWorld));
    m[1].NormalW = normalize(mul(0.5f * (inVerts[1].NormalL + inVerts[2].NormalL), (float3x3) gWorld));
    m[2].NormalW = normalize(mul(0.5f * (inVerts[0].NormalL + inVerts[2].NormalL), (float3x3) gWorld));

    float4 texC0 = mul(float4(0.5f * (inVerts[0].TexC + inVerts[1].TexC), 0.0f, 1.0f), gTexTransform);
    float4 texC1 = mul(float4(0.5f * (inVerts[1].TexC + inVerts[2].TexC), 0.0f, 1.0f), gTexTransform);
    float4 texC2 = mul(float4(0.5f * (inVerts[0].TexC + inVerts[2].TexC), 0.0f, 1.0f), gTexTransform);
    m[0].TexC = mul(texC0, gMatTransform).xy;
    m[1].TexC = mul(texC1, gMatTransform).xy;
    m[2].TexC = mul(texC2, gMatTransform).xy;
    
    VertexPosHWNormalTex v[3];
    [unroll]
    for (int i = 0; i < 3; ++i)
    {
        float4 posW = mul(float4(inVerts[i].PosL, 1.0f), gWorld);
        v[i].PosW = posW.xyz;
        v[i].PosH = mul(posW, gViewProj);
	
        float4 texC = mul(float4(inVerts[i].TexC, 0.0f, 1.0f), gTexTransform);
        v[i].TexC = mul(texC, gMatTransform).xy;
	
        v[i].NormalW = mul(inVerts[i].NormalL, (float3x3) gWorld);
    }
    
    outVerts[0] = v[0];
    outVerts[1] = m[0];
    outVerts[2] = m[2];
    outVerts[3] = m[1];
    outVerts[4] = v[2];
    outVerts[5] = v[1];
    
}

[maxvertexcount(12)]
void Sphere_GS(triangle VertexIn gin[3],
	inout TriangleStream<VertexPosHWNormalTex> triStream)
{
    VertexPosHWNormalTex v[6];
    Subdivide(gin, v);
    
    float3 toEyeW = gEyePosW - v[0].PosW;
    float distToEye = length(toEyeW);
    
    if (distToEye < 15.0f)
    {
        triStream.Append(v[0]);
        triStream.Append(v[1]);
        triStream.Append(v[2]);
        triStream.Append(v[3]);
        triStream.Append(v[4]);
        triStream.RestartStrip();
    
        triStream.Append(v[1]);
        triStream.Append(v[5]);
        triStream.Append(v[3]);
        
    }
    if(distToEye >= 15.0f)
    {
        triStream.Append(v[0]);
        triStream.Append(v[5]);
        triStream.Append(v[4]);
    }
    
}

[maxvertexcount(2)]
void VertexNormal_GS(point VertexIn gin[1],
	inout LineStream<VertexPosHWNormalTex> triStream)
{
    VertexPosHWNormalTex v[2];
    
    float4 posW = mul(float4(gin[0].PosL, 1.0f), gWorld);
    v[0].PosW = posW.xyz;
    v[0].NormalW = mul(gin[0].NormalL, (float3x3) gWorld);
    v[1].PosW = posW.xyz + v[0].NormalW * 1.0f;
    v[1].NormalW = v[0].NormalW;
    
    [unroll]
    for (int i = 0; i < 2; ++i)
    {
        float4 texC = mul(float4(gin[0].TexC, 0.0f, 1.0f), gTexTransform);
        v[i].TexC = mul(texC, gMatTransform).xy;
        v[i].PosH = mul(float4(v[i].PosW, 1.0f), gViewProj);
        triStream.Append(v[i]);
    }
    triStream.RestartStrip();
}


[maxvertexcount(2)]
void PlaneNormal_GS(triangle VertexIn gin[3],
	inout LineStream<VertexPosHWNormalTex> triStream)
{
    VertexPosHWNormalTex v[2];
    
    float4 posW = mul(float4((gin[0].PosL + gin[1].PosL + gin[2].PosL) * 0.33f, 1.0f), gWorld);
    v[0].PosW = posW.xyz;
    v[0].NormalW = mul(gin[0].NormalL + gin[1].NormalL + gin[2].NormalL, (float3x3) gWorld);
    v[1].PosW = posW.xyz + v[0].NormalW * 1.0f;
    v[1].NormalW = v[0].NormalW;
    
    [unroll]
    for (int i = 0; i < 2; ++i)
    {
        float4 texC = mul(float4(gin[0].TexC, 0.0f, 1.0f), gTexTransform);
        v[i].TexC = mul(texC, gMatTransform).xy;
        v[i].PosH = mul(float4(v[i].PosW, 1.0f), gViewProj);
        triStream.Append(v[i]);
    }
    triStream.RestartStrip();
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
