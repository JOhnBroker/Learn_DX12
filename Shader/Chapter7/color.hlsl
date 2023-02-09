
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float g_Pad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float g_DeltaTime;
};

struct VertexIn
{
    float3 PosL : POSITION;
    float4 Color : COLOR;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vOut;
    
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vOut.PosH = mul(posW, gViewProj);
    
    vOut.Color = vin.Color;
    
    return vOut;
}

float4 PS(VertexOut pin) : SV_Target
{
    return pin.Color;    
}