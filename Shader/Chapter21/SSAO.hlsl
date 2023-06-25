#ifndef SSAO_HLSL
#define SSAO_HLSL
#include "FullScreenTriangle.hlsl"

cbuffer cbSsao : register(b0)
{
    float4x4 gProjTex; // View to Texture Space
    // 远平面三角形（覆盖四个角点）
    float4 gFarPlanePoints[3];
    
    // 14个方向均匀分布但长度随机的向量
    float4 gOffsetVectors[14];
    
    // 观察空间下的坐标
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gOcclusionEpsilon;
    
    // SSAO_Blur
    float4 gBlurWeights[3];
    
    static float sBlurWeights[12] = (float[12]) gBlurWeights;
    
    float2 gTexelSize; // (1.0f / W, 1.0f / H)
    int gBlurRadius;
    float g_Pad;
    
};

Texture2D gNormalDepthMap : register(t0);
Texture2D gRandomVecMap : register(t1);
Texture2D gInputImage : register(t2);

SamplerState gSamLinearWrap : register(s0);
SamplerState gSamLinearClamp : register(s1);
SamplerState gSamNormalDepth : register(s2);
SamplerState gSamRandomVec : register(s3);
SamplerState gSamBlur : register(s4);

// 计算AO

float OcclusionFunction(float distZ)
{
    // 如果depth(q)在depth(p)之后(超出半球范围)，那点q不能遮蔽点p。此外，如果depth(q)和depth(p)过于接近，
    // 我们也认为点q不能遮蔽点p，因为depth(p)-depth(r)需要超过用户假定的Epsilon值才能认为点q可以遮蔽点p
    //
    // 我们用下面的函数来确定遮蔽程度
    //
    //    /|\ Occlusion
    // 1.0 |      ---------------\
    //     |      |             |  \
    //     |                         \
    //     |      |             |      \
    //     |                             \
    //     |      |             |          \
    //     |                                 \
    // ----|------|-------------|-------------|-------> zv
    //     0     Eps          zStart         zEnd
    float occlusion = 0.0f;
    if (distZ > gOcclusionEpsilon)
    {
        float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
        occlusion = saturate((gOcclusionEpsilon - distZ) / fadeLength);
    }
    return occlusion;
}

void VS(uint vertexID : SV_VertexID,
        out float4 posH : SV_Position,
        out float3 farPlanePoint : POSITION,
        out float2 texcoord : TEXCOORD)
{
    float2 grid = float2((vertexID << 1) & 2, vertexID & 2);
    float2 xy = grid * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    
    texcoord = grid * float2(1.0f, 1.0f);
    posH = float4(xy, 1.0f, 1.0f);
    farPlanePoint = gFarPlanePoints[vertexID].xyz;
}

float4 PS(float4 posH : SV_Position,
        float3 farPlanePoint : POSITION,
        float2 texcoord : TEXCOORD,
        uniform int sampleCount) : SV_Target
{
    // 1. 获取法向量和深度
    float4 normalDepth = gNormalDepthMap.SampleLevel(gSamNormalDepth, texcoord, 0.0f);
    float3 n = normalDepth.xyz;
    float depth = normalDepth.w;
    
    // 2. 重建观察空间坐标
    // p : 计算的环境光遮蔽目标点
    float3 p = (depth / farPlanePoint.z) * farPlanePoint;
    
    // 3. 获取随机向量并从[0, 1]^3映射到[-1, 1]^3
    float3 randVec = gRandomVecMap.SampleLevel(gSamRandomVec, 4.0f * texcoord, 0.0f).xyz;
    randVec = 2.0f * randVec - 1.0f;
    
    float occlusionSum = 0.0f;
    
    for (int i = 0; i < sampleCount; ++i)
    {
        float3 offset = reflect(gOffsetVectors[i].xyz, randVec);
        
        float flip = sign(dot(offset, n));
        
        // 对p点除遮蔽半径的半球范围进行采样
        float3 q = p + flip * gOcclusionRadius * offset;
        // 将q投影到纹理坐标
        float4 projQ = mul(float4(q, 1.0f), gProjTex);
        projQ /= projQ.w;
        
        // 查询此点在深度图的值
        float rz = gNormalDepthMap.SampleLevel(gSamNormalDepth, projQ.xy, 0.0f).w;
        // 重建r点在视察空间中的坐标
        float3 r = (rz / q.z) * q;
        
        // 判断是否遮蔽p点
        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);
        float occlusion = dp * OcclusionFunction(distZ);
        
        occlusionSum += occlusion;
    }
    occlusionSum /= sampleCount;
    
    float access = 1.0f - occlusionSum;
    
    return saturate(pow(access, 6.0f));
}

// 模糊AO
// 双边滤波

float4 BilateralPS(float4 posH : SV_Position,
                    float2 texcoord) : SV_Target
{
    // 把中心值加进去计算
    float4 color = sBlurWeights[gBlurRadius] * gInputImage.SampleLevel(gSamBlur, texcoord, 0.0f);
    float totalWeight = sBlurWeights[gBlurRadius];
    
    float4 centerNormalDepth = gNormalDepthMap.SampleLevel(gSamBlur, texcoord, 0.0f);
    float3 centerNormal = centerNormalDepth.xyz;
    float centerDepth = centerNormalDepth.w;
    
    for (int i = -gBlurRadius; i < gBlurRadius; ++i)
    {
        if (i == 0)
            continue;
#if defined BLUR_HORZ
        float2 offset = float2(gTexelSize.x * i, 0.0f);
#else
        float2 offset = float2(0.0f, gTexelSize.y * i);
#endif
        float4 neighborNormalDepth = gNormalDepthMap.SampleLevel(gSamBlur, texcoord + offset, 0.0f);
        float3 neighborNormal = neighborNormalDepth.xyz;
        float neighborDepth = neighborNormalDepth.w;
        
        // 如果中心值和相邻值的深度或法向量相差太大，我们就认为当前采样点处于边缘区域，
        // 因此不考虑加入当前相邻值
        // 中心值直接加入
        if (dot(neighborNormal,centerNormal)>=0.8f && 
            abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = sBlurWeights[i + gBlurRadius];
            // 将相邻像素加入进行模糊
            color += weight * gInputImage.SampleLevel(gSamBlur, texcoord + offset, 0.0f);
            totalWeight += weight;
        }
        
    }
    return color / totalWeight;
}

float4 DebugAO_PS(float4 posH : SV_Position,
                float2 texcoord : TEXCOORD) : SV_Target
{
    float depth = gInputImage.Sample(gSamLinearWrap, texcoord).r;
    return float4(depth.rrr, 1.0f);
}

#endif //SSAO_HLSL