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

enum class ResourceFlag 
{
	SHADER_RESOURCE = 0x8L,
	RENDER_TARGET = 0x20L,
	DEPTH_STENCIL = 0x40L,
	UNORDERED_ACCESS = 0x80L
};

class ITexture 
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ITexture(ID3D12Device* device, const D3D12_RESOURCE_DESC& texDesc);
	virtual ~ITexture() = 0 {};

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

class Texture2D :public ITexture 
{
public:
	Texture2D(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t mipLevels = 1, uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2D()override = default;

	// 不允许拷贝，允许移动
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(Texture2D&) = delete;
	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void GetDescriptorCount(int& srvCount, int rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() 
	{
		if (m_nRtvCount)
			return m_hCpuRtv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	};
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUnorderedAccess() 
	{ 
		if (m_nSrvCount > 1)
			return m_hGpuUav;
		return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	};

	uint32_t GetMiopLevels() const { return m_MipLevels; };

protected:
	// 默认创建Shader Resource View
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	uint32_t m_MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuUav = {};
};

class Texture2DMS :public ITexture 
{
public:
	Texture2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
		DXGI_FORMAT format, const DXGI_SAMPLE_DESC& sampleDesc,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2DMS()override = default;

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() 
	{
		if (m_nRtvCount)
			return m_hCpuRtv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	uint32_t GetMsaaSamples()const { return m_MsaaSamples; }

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	uint32_t m_MsaaSamples = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv = {};
};

class TextureCube :public ITexture 
{
public:
	TextureCube(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t mipLevels = 1,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~TextureCube() override = default;

	uint32_t GetMipLevels() const { return m_MipLevels; }
	uint32_t GetArraySize() const { return m_ArraySize; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() { return m_TextureArrayRTV; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) { return m_UnorderedAccessElements[arrayIndex]; }
	//RWTexture2D
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccess(size_t arrayIndex) { return m_UnorderedAccessElements[arrayIndex]; }

	//Texture2DArray 
	// ITexture::GetRenderTarget
	//Texture2D
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) { return m_ShaderResourceElements[arrayIndex]; }

protected:
	uint32_t m_MipLevels = 1;
	uint32_t m_ArraySize = 1;
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_UnorderedAccessElements = {};
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

class Texture2DArray :public ITexture 
{
public:
	Texture2DArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t arraySize, uint32_t mipLevels = 1,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2DArray() override = default;

	uint32_t GetMipLevels() const { return m_MipLevels; }
	uint32_t GetArraySize() const { return m_ArraySize; }
	
	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() { return m_TextureArrayRTV; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) { return m_RenderTargetElements[arrayIndex]; }
	//RWTexture2D
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccess(size_t arrayIndex) { return m_UnorderedAccessElements[arrayIndex]; }
	//Texture2DArray
	// ITexture::GetRenderTarget
	//Texture2D
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) { return m_ShaderResourceElements[arrayIndex]; }

protected:
	uint32_t m_MipLevels = 1;
	uint32_t m_ArraySize = 1;
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_UnorderedAccessElements = {};
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

class Texture2DMSArray :public ITexture 
{
public:
	Texture2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t mipLevels = 1,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2DMSArray() override = default;

	uint32_t GetArraySize() const { return m_ArraySize; }
	uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() { return m_TextureArrayRTV; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) { return m_RenderTargetElements[arrayIndex]; }
	//RWTexture2D
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccess(size_t arrayIndex) { return m_UnorderedAccessElements[arrayIndex]; }
	//Texture2DArray
	// ITexture::GetRenderTarget
	//Texture2D
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) { return m_ShaderResourceElements[arrayIndex]; }

protected:
	uint32_t m_ArraySize = 1;
	uint32_t m_MsaaSamples = 1;
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_UnorderedAccessElements = {};
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

class Depth2D :public ITexture {};

class Depth2DMS :public ITexture {};

class Depth2DArray : public ITexture{};

class Depth2DMSArray :public ITexture {};

#endif // !TEXTURE_H

