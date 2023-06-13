#ifndef TEXTURE_H
#define TEXTURE_H

#include "d3dUtil.h"

struct Texture 
{
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	std::string m_Name;
	std::wstring m_Filename;

	ComPtr<ID3D12Resource> m_Resource = nullptr;
	ComPtr<ID3D12Resource> m_UploadHeap = nullptr;
};

class ITexture 
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ITexture(ID3D12Device* device, const D3D12_RESOURCE_DESC& texDesc);
	virtual ~ITexture() = 0;

	ID3D12Resource* GetTexture() { return m_Resource.Get(); }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetSRV() { return m_hGpuSrv; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }

	void BuildDescriptor(ID3D12Device* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv);

protected:
	ComPtr<ID3D12Resource> m_Resource = nullptr;
	ComPtr<ID3D12Resource> m_UploadHeap = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv;
	uint32_t m_Width = 0, m_Height = 0;

};

class Texture2D :public ITexture {};

class Texture2DMS :public ITexture {};

class TextureCube :public ITexture {};

class Texture2DArray :public ITexture {};

class Texture2DMSArray :public ITexture {};

class Depth2D :public ITexture {};

class Depth2DMS :public ITexture {};

class Depth2DArray : public ITexture{};

class Depth2DMSArray :public ITexture {};

#endif // !TEXTURE_H

