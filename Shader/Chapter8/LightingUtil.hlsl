#define MaxLights 16

struct Light
{
	float3 Strength;
	float FalloffStart;
	float3 Direction;
	float FalloffEnd;
	float3 Position;
	float SpotPower;
};

struct Material
{
    float4 DiffuseAlbedo;
	float3 FresnelR0;
	float Shininess;
};

// 计算线性衰减
float CalcAttenuation(float dist,float falloffStart,float falloffEnd)
{
	return saturate((falloffEnd - dist)/(falloffEnd - falloffStart));
}

// 计算菲涅尔反射率的近似方法
float3 SchlickFresnel(float3 R0,float3 normal,float3 lightVec)
{
	// R0 = ((n-1)/(n+1))^2 ,其中n为折射率
	// 入射角
	float cosIncidentAngle = saturate(dot(normal,lightVec));
	
	float f0 = 1.0f-cosIncidentAngle;
	float3 reflectPercent = R0 +(1.0f-R0)*(f0*f0*f0*f0*f0);
	return reflectPercent;
}

float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
	// 计算粗糙度，为什么需要乘255？
	const float m = mat.Shininess * 256.0f;

	float3 halfVec = normalize(toEye + lightVec);
	
	// 计算微平面理论的反射率
	float roughnessFactor = (m+8.0f)* pow(max(dot(halfVec,normal), 0.0f), m) / 8.0f;
	// 计算菲涅尔反射率
	float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
	
	float3 specAlbedo = fresnelFactor * roughnessFactor;
	specAlbedo = specAlbedo / (specAlbedo + 1.0f);
	
    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

// 计算直接光照
float3 ComputeDirectionalLight(Light L, Material mat,float3 normal,float3 toEye)
{
	float3 lightVec = -L.Direction;
	
	float ndotl = max(dot(normal,lightVec),0.0f);
	float3 lightStrength = L.Strength * ndotl;
	
	return BlinnPhong(lightStrength,lightVec,normal,toEye,mat);
}

// 计算点光源
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = {0.0f, 0.0f, 0.0f};
	float3 lightVec = L.Position - pos;
	
	float dist = length(lightVec);
	
	[flatten]
	if(dist > L.FalloffEnd)
	{
		return result;
	}

	lightVec /= dist;
	
	float ndotl = max(dot(normal,lightVec),0.0f);
	float3 lightStrength = L.Strength * ndotl;
	
	// 衰减量
	float att = CalcAttenuation(dist, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;

	result = BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
	return result;
}

// 计算聚光灯
float3 ComputeSpotLight(Light L,Material mat, float3 pos, float3 normal, float3 toEye)
{
	float3 result = {0.0f, 0.0f, 0.0f};
	float3 lightVec = L.Position - pos;
	
	float dist = length(lightVec);
	
	[flatten]
	if(dist > L.FalloffEnd)
	{
		return result;
	}

	lightVec /= dist;
	
	float ndotl = max(dot(normal,lightVec),0.0f);
	float3 lightStrength = L.Strength * ndotl;
	
	float att = CalcAttenuation(dist, L.FalloffStart, L.FalloffEnd);
	lightStrength *= att;
	
	// 聚光灯衰减
	float spotFactor = pow( max( dot( -lightVec, L.Direction ), 0.0f ), L.SpotPower);
	lightStrength *= spotFactor;
	
	result = BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
	return result;
}

// 光照计算函数
float4 ComputeLighting(Light gLights[MaxLights], Material mat,
						float3 pos, float3 normal, float3 toEye,
						float3 shadowFactor)
{
	float3 result = {0.0f, 0.0f, 0.0f};
	
	int i = 0;
	
#if (NUM_DIR_LIGHT > 0)
	for(i = 0; i< NUM_DIR_LIGHT; ++i)
	{
		result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
	}
#endif

#if (NUM_POINT_LIGHT > 0)
	for(i = NUM_DIR_LIGHT; i< NUM_DIR_LIGHT + NUM_POINT_LIGHT; ++i)
	{
		result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
	}
#endif

#if (NUM_SPOT_LIGHT > 0)
	for(i = NUM_DIR_LIGHT + NUM_POINT_LIGHT; i< NUM_DIR_LIGHT + NUM_POINT_LIGHT + NUM_SPOT_LIGHT; ++i)
	{
		result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
	}
#endif

	return float4(result, 0.0f);
}

