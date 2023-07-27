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
#ifdef _DEBUG
	return m_Textures.try_emplace(name, texture).second;
#else
	XID nameID = StringToID(name);
	return m_Textures.try_emplace(nameID, texture).second;
#endif // _DEBUG
}

void TextureManager::RemoveTexture(std::string name)
{
#ifdef _DEBUG
	RemoveTextureIndex(name);
	m_Textures.erase(name);
#else
	XID nameID = StringToID(name);
	RemoveTextureIndex(name);
	m_Textures.erase(nameID);
#endif // _DEBUG
}

ITexture* TextureManager::GetTexture(std::string name)
{
#ifdef _DEBUG
	if (m_Textures.count(name))
	{
		return m_Textures[name].get();
	}
	return nullptr;
#else
	XID nameID = StringToID(name);
	if (m_Textures.count(nameID)) 
	{
		return m_Textures[nameID].get();
	}
	return nullptr;	
#endif // _DEBUG
}

// when texture don't exit, return -1 
int TextureManager::GetTextureSrvIndex(std::string name)
{
#ifdef _DEBUG
	if (m_TexturesIndex.count(name))
	{
		return m_TexturesIndex[name][0];
	}
	return -1;
#else
	XID nameID = StringToID(name);
	if (m_TexturesIndex.count(nameID))
	{
		return m_TexturesIndex[nameID][0];
	}
	return -1;
#endif // _DEBUG
}

// when texture don't exit, return -1 
int TextureManager::GetTextureDsvIndex(std::string name)
{
#ifdef _DEBUG
	if (m_TexturesIndex.count(name))
	{
		return m_TexturesIndex[name][1];
	}
	return -1;
#else
	XID nameID = StringToID(name);
	if (m_TexturesIndex.count(nameID))
	{
		return m_TexturesIndex[nameID][1];
	}
	return -1;
#endif // _DEBUG
}

// when texture don't exit, return -1 
int TextureManager::GetTextureRtvIndex(std::string name)
{
#ifdef _DEBUG
	if (m_TexturesIndex.count(name))
	{
		return m_TexturesIndex[name][2];
	}
	return -1;
#else
	XID nameID = StringToID(name);
	if (m_TexturesIndex.count(nameID))
	{
		return m_TexturesIndex[nameID][2];
	}
	return -1;
#endif // _DEBUG
}

void TextureManager::BuildDescriptor(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	// dsvIndex 和 rtvIndex 从0开始使用
	int srvIndex = 1, dsvIndex = -1, rtvIndex = -1;
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
		int curDsvIndex = it.second->GetDSVDescriptorCount() > 0 ? dsvIndex += it.second->GetDSVDescriptorCount(), curDsvIndex = dsvIndex : -1;
		int curRtvIndex = it.second->GetRTVDescriptorCount() > 0 ? rtvIndex += it.second->GetRTVDescriptorCount(), curRtvIndex = rtvIndex : -1;
		AddTextureIndex(it.first, { srvIndex++,dsvIndex ,rtvIndex });
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
	AddTextureIndex("nullTex", { srvIndex++,-1 ,-1 });

	nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	nullDesc.Format = DXGI_FORMAT_BC1_UNORM;
	nullDesc.TextureCube.MostDetailedMip = 0;
	nullDesc.TextureCube.MipLevels = -1;
	nullDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	m_pDevice->CreateShaderResourceView(nullptr, &nullDesc, hCpuSrv);
	m_NullCubeTex = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);
	AddTextureIndex("nullCubeTex", { srvIndex++,-1 ,-1 });

	m_nSRVCount += 2;

	for (auto& it : m_Textures)
	{
		m_nSRVCount += it.second->GetSRVDescriptorCount();
		m_nDSVCount += it.second->GetDSVDescriptorCount();
		m_nRTVCount += it.second->GetRTVDescriptorCount();
	}
}

void TextureManager::ReBuildDescriptor(std::string name, std::shared_ptr<ITexture> texture)
{
	ITexture* oldTexture = GetTexture(name);
	INT srvIndex = GetTextureSrvIndex(name), dsvIndex = GetTextureDsvIndex(name), rtvIndex = GetTextureRtvIndex(name);
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv = m_hCpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv = m_hGpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv = m_hCpuDsv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv = m_hCpuRtv;

	texture->BuildDescriptor(m_pDevice.Get(), 
		hCpuSrv.Offset(srvIndex, m_uiSrvDescriptorSize), hGpuSrv.Offset(srvIndex, m_uiSrvDescriptorSize),
		hCpuDsv.Offset(dsvIndex, m_uiDsvDescriptorSize), hCpuRtv.Offset(rtvIndex, m_uiRtvDescriptorSize),
		m_uiSrvDescriptorSize, m_uiDsvDescriptorSize, m_uiRtvDescriptorSize);

	RemoveTexture(name);
	AddTexture(name, texture);
	AddTextureIndex(name, { srvIndex,dsvIndex ,rtvIndex });
}

bool TextureManager::AddTextureIndex(std::string name, std::array<int, 3> index)
{
#ifdef _DEBUG
	return m_TexturesIndex.try_emplace(name, index).second;
#else
	XID nameID = StringToID(name);
	return m_TexturesIndex.try_emplace(nameID, index).second;
#endif // _DEBUG

}

bool TextureManager::AddTextureIndex(XID nameID, std::array<int, 3> index)
{
#ifdef _DEBUG
	return false;
	//return m_TexturesIndex.try_emplace(nameID, index).second;
#else
	return m_TexturesIndex.try_emplace(nameID, index).second;
#endif // _DEBUG
}

void TextureManager::RemoveTextureIndex(std::string name)
{
#ifdef _DEBUG
	m_TexturesIndex.erase(name);
#else
	XID nameID = StringToID(name);
	m_TexturesIndex.erase(nameID);
#endif // _DEBUG
}
