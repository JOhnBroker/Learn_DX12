
cbuffer cbSettings : register(b0)
{
    int gBlurRadius;
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

static const int gMaxBlurRadius = 5;

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);

#define N 256
#define CacheSize (N + 2*gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N,1,1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dtid : SV_DispatchThreadID)
{
    // one group thread shared one cache, so gCache index by SV_GroupThreadID
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // write gCache
    if (groupThreadID.x < gBlurRadius)
    {
        int pos = max(dtid.x - gBlurRadius, 0);
        gCache[groupThreadID.x] = gInput[int2(pos, dtid.y)];
    }
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int pos = min(dtid.x + gBlurRadius, gInput.Length.x - 1);
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(pos, dtid.y)];
    }
    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dtid.xy, gInput.Length.xy - 1)];
    
    // Sync
    GroupMemoryBarrierWithGroupSync();
    
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int pos = groupThreadID.x + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[pos];
    }
    
    gOutput[dtid.xy] = blurColor;
}

[numthreads(1,N,1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dtid : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // write gCache
    if (groupThreadID.y < gBlurRadius)
    {
        int pos = max(dtid.y - gBlurRadius, 0);
        gCache[groupThreadID.y] = gInput[int2(dtid.x,pos)];
    }
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int pos = min(dtid.y + gBlurRadius, gInput.Length.y - 1);
        gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dtid.x, pos)];
    }
    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dtid.xy, gInput.Length.xy - 1)];
    
    // Sync
    GroupMemoryBarrierWithGroupSync();
    
    float4 blurColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int pos = groupThreadID.y + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[pos];
    }
    
    gOutput[dtid.xy] = blurColor;
}