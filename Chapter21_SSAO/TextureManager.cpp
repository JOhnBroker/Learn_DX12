#include "TextureManager.h"

namespace 
{
	// TextureManager 单例
	TextureManager* s_pInstance = nullptr;
}


TextureManager::TextureManager()
{
	if (s_pInstance) 
	{
		throw std::exception("TextureManager is a singleton!");
	}
	s_pInstance = this;
}

TextureManager::~TextureManager()
{
}

TextureManager& TextureManager::Get()
{
	if (!s_pInstance)
	{
		throw std::exception("TextureManager need an instance!");
	}
	return *s_pInstance;
}

void TextureManager::Init(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	m_pDevice = device;
	m_pCommandList = cmdList;
}

ITexture* TextureManager::CreateFromeFile(std::string filename, std::string name, bool isCube, bool forceSRGB) 
{
	uint32_t width = 0, height = 0, mipLevels = 1;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> uploadHeap;

	XID nameID = StringToID(filename);
	std::wstring wstrName = UTF8ToWString(filename);

	HR(DirectX::CreateDDSTextureFromFile12(m_pDevice.Get(), m_pCommandList.Get(),
			wstrName.c_str(), resource, uploadHeap));

	width		= resource->GetDesc().Width;
	height		= resource->GetDesc().Height;
	mipLevels	= resource->GetDesc().MipLevels;
	format		= resource->GetDesc().Format;

	if (isCube) 
	{
		auto texCube = std::make_shared<TextureCube>(m_pDevice.Get(), width, height,
			format, mipLevels, (uint32_t)ResourceFlag::SHADER_RESOURCE,
			resource, uploadHeap);
		AddTexture(name, texCube);
	}
	else 
	{
		auto text2D = std::make_shared<Texture2D>(m_pDevice.Get(), width, height,
			format, mipLevels, (uint32_t)ResourceFlag::SHADER_RESOURCE,
			resource, uploadHeap);
		AddTexture(name, text2D);
	}
	return GetTexture(name);
}

ITexture* TextureManager::CreateFromeMemory(std::string name, void* data, size_t byteWidth, bool isCube, bool forceSRGB)
{
	XID nameID = StringToID(name);
	ComPtr<ID3D12Resource> resource;
	ComPtr<ID3D12Resource> uploadHeap;
	uint32_t width = 0, height = 0, mipLevels = 1;
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;

	HR(DirectX::CreateDDSTextureFromMemory12(m_pDevice.Get(), m_pCommandList.Get(),
		(const uint8_t*)data, byteWidth, resource, uploadHeap));

	width = resource->GetDesc().Width;
	height = resource->GetDesc().Height;
	mipLevels = resource->GetDesc().MipLevels;
	format = resource->GetDesc().Format;

	auto texture = std::make_shared<Texture2D>(m_pDevice.Get(), width, height,
		format, mipLevels, (uint32_t)ResourceFlag::SHADER_RESOURCE,
		resource, uploadHeap);
	AddTexture(name, texture);
	return GetTexture(name);
}

bool TextureManager::AddTexture(std::string name, std::shared_ptr<ITexture> texture)
{
	XID nameID = StringToID(name);
	return m_Textures.try_emplace(nameID, texture).second;
}

void TextureManager::RemoveTexture(std::string name)
{
	XID nameID = StringToID(name);
	RemoveTextureIndex(name);
	m_Textures.erase(nameID);
}

ITexture* TextureManager::GetTexture(std::string name)
{
	XID nameID = StringToID(name);
	if (m_Textures.count(nameID)) 
	{
		return m_Textures[nameID].get();
	}
	return nullptr;	
}

// when texture don't exit, return -1 
int TextureManager::GetTextureIndex(std::string name)
{
	XID nameID = StringToID(name);
	if (m_TexturesIndex.count(nameID))
	{
		return m_TexturesIndex[nameID];
	}
	return -1;
}

void TextureManager::BuildDescriptor(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	int index = 1;
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;
	m_hCpuDsv = hCpuDsv;
	m_hCpuRtv = hCpuRtv;

	m_uiSrvDescriptorSize = uiSrvDescriptorSize;
	m_uiDsvDescriptorSize = uiDsvDescriptorSize;
	m_uiRtvDescriptorSize = uiRtvDescriptorSize;

	for (auto& it : m_Textures)
	{
		it.second->BuildDescriptor(m_pDevice.Get(), hCpuSrv, hGpuSrv, hCpuDsv,
			hCpuRtv, uiSrvDescriptorSize, uiDsvDescriptorSize, uiRtvDescriptorSize);
		AddTextureIndex(it.first, index++);
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC nullDesc;
	nullDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	nullDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	nullDesc.Texture2D.MostDetailedMip = 0;
	nullDesc.Texture2D.MipLevels = -1;
	nullDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	nullDesc.Texture2D.PlaneSlice = 0;

	m_pDevice->CreateShaderResourceView(nullptr, &nullDesc, hCpuSrv);
	m_NullTex = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);
	AddTextureIndex("nullTex", index++);

	nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	nullDesc.Format = DXGI_FORMAT_BC1_UNORM;
	nullDesc.TextureCube.MostDetailedMip = 0;
	nullDesc.TextureCube.MipLevels = -1;
	nullDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	m_pDevice->CreateShaderResourceView(nullptr, &nullDesc, hCpuSrv);
	m_NullCubeTex = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);
	AddTextureIndex("nullCubeTex", index++);

	m_nSRVCount += 2;

	for (auto& it : m_Textures)
	{
		m_nSRVCount += it.second->GetSRVDescriptorCount();
		m_nDSVCount += it.second->GetDSVDescriptorCount();
		m_nRTVCount += it.second->GetRTVDescriptorCount();
	}
}

void TextureManager::ReBuildDescriptor(std::string name, UINT oldIndex)
{
	ITexture* texture = GetTexture(name);
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv = m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv = m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv = m_hCpuDsv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv = m_hCpuRtv;

	AddTextureIndex(name, oldIndex);
	texture->BuildDescriptor(m_pDevice.Get(), hCpuSrv.Offset(oldIndex,m_uiSrvDescriptorSize), hGpuSrv.Offset(oldIndex,m_uiSrvDescriptorSize),
		hCpuDsv.Offset(1, m_uiDsvDescriptorSize), hCpuRtv, m_uiSrvDescriptorSize, m_uiDsvDescriptorSize, m_uiRtvDescriptorSize);
}

void TextureManager::ReBuildDescriptor(std::string name, UINT oldIndex, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, 
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	ITexture* texture = GetTexture(name);

	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv = m_hCpuSrv;

	AddTextureIndex(name, oldIndex);
	texture->BuildDescriptor(m_pDevice.Get(), hCpuSrv.Offset(oldIndex, m_uiSrvDescriptorSize), hGpuSrv, hCpuDsv, hCpuRtv, 
		m_uiSrvDescriptorSize, m_uiDsvDescriptorSize, m_uiRtvDescriptorSize);
}

bool TextureManager::AddTextureIndex(std::string name, int index)
{
	XID nameID = StringToID(name);
	return m_TexturesIndex.try_emplace(nameID, index).second;
}

bool TextureManager::AddTextureIndex(XID nameID, int index)
{
	return m_TexturesIndex.try_emplace(nameID, index).second;
}

void TextureManager::RemoveTextureIndex(std::string name)
{
	XID nameID = StringToID(name);
	m_TexturesIndex.erase(nameID);
}
