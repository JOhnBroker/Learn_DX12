#include "TextureManager.h"

namespace 
{
	// TextureManager µ¥Àý
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

void TextureManager::Init(ID3D12Device* device)
{
	m_pDevice = device;
	
}

ID3D12Resource* TextureManager::CreateFromeFile(std::string filename, bool enableMips, bool forceSRGB)
{
	return nullptr;
}

ID3D12Resource* TextureManager::CreateFromeMemory(std::string name, void* data, size_t byteWidth, bool enableMips, bool forceSRGB)
{
	return nullptr;
}

bool TextureManager::AddTexture(std::string name, ID3D12Resource* texture)
{
	return false;
}

void TextureManager::RemoveTexture(std::string name)
{
}

ID3D12Resource* TextureManager::GetTexture(std::string name)
{
	return nullptr;
}

ID3D12Resource* TextureManager::GetNullTexture()
{
	return nullptr;
}

void TextureManager::BuildSRVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
}

void TextureManager::BuildRTVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
}

void TextureManager::BuildDSVDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
}

void TextureManager::BuildDescriptors()
{
}

void TextureManager::BuildResource()
{
}
