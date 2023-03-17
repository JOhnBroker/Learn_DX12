
struct CSInput
{
    float3 vec;
};

struct CSOutput
{
    float norm;
};

StructuredBuffer<CSInput> gStructInput : register(t0);
RWStructuredBuffer<CSOutput> gStructOutput : register(u0);

ConsumeStructuredBuffer<CSInput> gConsumeInoput : register(u1);
AppendStructuredBuffer<CSOutput> gAppendOutoput : register(u2);


[numthreads(32,1,1)]
void CS_Exe1(int3 dtid : SV_DispatchThreadID)
{
    gStructOutput[dtid.x].norm = length(gStructInput[dtid.x].vec);
}

[numthreads(32,1,1)]
void CS_Exe2()
{
    CSOutput cout;
    CSInput cin = gConsumeInoput.Consume();
    
    cout.norm = length(cin.vec);
    gAppendOutoput.Append(cout);
}