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

	XID nameID = StringToID(filename);
	std::wstring wstrName = UTF8ToWString(filename);

	auto texture = std::make_shared<Texture>();
	HR(DirectX::CreateDDSTextureFromFile12(m_pDevice.Get(), m_pCommandList.Get(),
			wstrName.c_str(), texture->m_Resource, texture->m_UploadHeap));

	width = texture->m_Resource->GetDesc().Width;
	height = texture->m_Resource->GetDesc().Height;
	mipLevels = texture->m_Resource->GetDesc().MipLevels;
	format = texture->m_Resource->GetDesc().Format;

	if (isCube) 
	{
		auto texCube = std::make_shared<TextureCube>(m_pDevice.Get(), width, height,
			format, mipLevels, (uint32_t)ResourceFlag::SHADER_RESOURCE,
			texture->m_Resource, texture->m_UploadHeap);
		AddTexture(name, texCube);
	}
	else 
	{
		auto text2D = std::make_shared<Texture2D>(m_pDevice.Get(), width, height,
			format, mipLevels, (uint32_t)ResourceFlag::SHADER_RESOURCE,
			texture->m_Resource, texture->m_UploadHeap);
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

#if (defined(DEBUG) || defined(_DEBUG)) && (GRAPHICS_DEBUGGER_OBJECT_NAME)
	::D3D12SetDebugObjectName(texture->GetTexture().Get(), name.c_str());
#endif
	return m_Textures.try_emplace(nameID, texture).second;
}

void TextureManager::RemoveTexture(std::string name)
{
	XID nameID = StringToID(name);
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

void TextureManager::BuildDescriptor(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;
	m_hCpuDsv = hCpuDsv;
	m_hCpuRtv = hCpuRtv;

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

	nullDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	nullDesc.Format = DXGI_FORMAT_BC1_UNORM;
	nullDesc.TextureCube.MostDetailedMip = 0;
	nullDesc.TextureCube.MipLevels = -1;
	nullDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	m_pDevice->CreateShaderResourceView(nullptr, &nullDesc, hCpuSrv);
	m_NullCubeTex = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);

	m_nSRVCount += 2;

	for (auto& it : m_Textures) 
	{
		TextureType type = it.second->GetTextureType();
		it.second->BuildDescriptor(m_pDevice.Get(),hCpuSrv, hGpuSrv, hCpuDsv,
			hCpuRtv, uiSrvDescriptorSize, uiDsvDescriptorSize, uiRtvDescriptorSize);
	}

	for (auto& it : m_Textures)
	{
		m_nSRVCount += it.second->GetSRVDescriptorCount();
		m_nDSVCount += it.second->GetDSVDescriptorCount();
		m_nRTVCount += it.second->GetRTVDescriptorCount();
	}
}
