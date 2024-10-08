#ifndef MATERIAL_H
#define MATERIAL_H

#include "d3dUtil.h"
#include "FrameResource.h"
#include "MathHelper.h"
#include <string>

struct MaterialConstants 
{
	DirectX::XMFLOAT4 m_DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 m_FresnelR0 = { 0.01f,0.01f,0.01f };
	float Roughness = 0.25f;

	DirectX::XMFLOAT4X4 m_MatTransform = MathHelper::Identity4x4();
};

struct Material
{
	std::string m_Name;
	
	// 常量缓冲区 对应到该 材质的索引
	int m_MatCBIndex = -1;
	// Diffuse 贴图 SRV heap 的索引
	int m_DiffuseSrvHeapIndex = -1;
	// Normal 贴图 SRV heap 的索引
	int m_NormalSrvHeapIndex = -1;

	// 每个FrameResource均需要升级
	int m_NumFramesDirty = g_numFrameResources;

	DirectX::XMFLOAT4 m_DiffuseAlbedo = { 1.0f,1.0f,1.0f,1.0f };
	DirectX::XMFLOAT3 m_FresnelR0 = { 0.01f,0.01f,0.01f };
	float m_Roughness = 0.25f;
	DirectX::XMFLOAT4X4 m_MatTransform = MathHelper::Identity4x4();
};

#endif //MATERIAL_H