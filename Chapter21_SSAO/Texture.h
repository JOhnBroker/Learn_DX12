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
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource() { return m_hGpuSrv; }

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

	uint32_t GetMiopLevels() const { return m_MipLevels; };

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT uiSrvDescriptorSize);
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

	uint32_t GetMsaaSamples()const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv, UINT uiSrvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() 
	{
		if (m_nRtvCount)
			return m_hCpuRtv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

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

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiRtvvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount) 
			return m_TextureArrayRTV;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) { 
		if (arrayIndex < m_RenderTargetElements.size())
			return m_RenderTargetElements[arrayIndex];
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	//RWTexture2D
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUnorderedAccess(size_t arrayIndex) {
		if (arrayIndex < m_UnorderedAccessElements.size())
			return m_UnorderedAccessElements[arrayIndex];
		return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	}
	//TextureCube
	using ITexture::GetShaderResource;
	////Texture2D
	//CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) {
	//	if (arrayIndex < m_ShaderResourceElements.size())
	//		return m_ShaderResourceElements[arrayIndex];
	//	return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	//}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	uint32_t m_MipLevels = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_UnorderedAccessElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
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
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiRtvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount)
			return m_TextureArrayRTV;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) {
		if (arrayIndex < m_RenderTargetElements.size())
			return m_RenderTargetElements[arrayIndex];
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	//RWTexture2D
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUnorderedAccess(size_t arrayIndex) {
		if (arrayIndex < m_UnorderedAccessElements.size())
			return m_UnorderedAccessElements[arrayIndex];
		return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	}
	//Texture2DArray 
	using ITexture::GetShaderResource;
	////Texture2D
	//CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) {
	//	if (arrayIndex < m_ShaderResourceElements.size())
	//		return m_ShaderResourceElements[arrayIndex];
	//	return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	//}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	uint32_t m_MipLevels = 1;
	uint32_t m_ArraySize = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_UnorderedAccessElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

class Texture2DMSArray :public ITexture 
{
public:
	Texture2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, 
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2DMSArray() override = default;

	uint32_t GetArraySize() const { return m_ArraySize; }
	uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiRtvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& rtvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount)
			return m_TextureArrayRTV;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget(size_t arrayIndex) {
		if (arrayIndex < m_RenderTargetElements.size())
			return m_RenderTargetElements[arrayIndex];
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	//Texture2DMSArray 
	using ITexture::GetShaderResource;
	////Texture2DMS
	//CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) {
	//	if (arrayIndex < m_ShaderResourceElements.size())
	//		return m_ShaderResourceElements[arrayIndex];
	//	return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	//}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nRtvCount = 0;
	uint32_t m_ArraySize = 1;
	uint32_t m_MsaaSamples = 1;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_TextureArrayRTV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_RenderTargetElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};


enum class DepthStencilBitsFlag
{
	Depth_16Bits = 0,
	Depth_24Bits_Stencil_8Bits = 1,
	Depth_32Bits = 2,
	Depth_32Bits_Stencil_8Bits_Unused_24Bits = 3,
};

class Depth2D :public ITexture 
{
public:
	Depth2D(ID3D12Device* device, uint32_t width, uint32_t height, 
		DepthStencilBitsFlag  depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits, 
		uint32_t resourceFlag = (uint32_t)ResourceFlag::DEPTH_STENCIL | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Depth2D() override = default;

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& dsvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() 
	{
		if (m_nDsvCount)
			return m_DepthStencilView;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nDsvCount = 0;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_DepthStencilView = {};
};

class Depth2DMS :public ITexture 
{
public:
	Depth2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
		const DXGI_SAMPLE_DESC& sampleDesc,
		DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::DEPTH_STENCIL | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Depth2DMS() override = default;

	uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& dsvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil()
	{
		if (m_nDsvCount)
			return m_DepthStencilView;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nDsvCount = 0;
	uint32_t m_MsaaSamples = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_DepthStencilView = {};
};

class Depth2DArray : public ITexture
{
public:
	Depth2DArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
		DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits, 
		uint32_t resourceFlag = (uint32_t)ResourceFlag::DEPTH_STENCIL | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Depth2DArray() override = default;

	uint32_t GetArraySize() const { return m_ArraySize; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& dsvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() {
		if (m_nDsvCount)
			return m_DepthArrayDSV;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil(size_t arrayIndex) {
		if (arrayIndex < m_DepthStencilElements.size())
			return m_DepthStencilElements[arrayIndex];
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	// TextureArray
	using ITexture::GetShaderResource;
	//// Texture2D
	//CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) {
	//	if (arrayIndex < m_ShaderResourceElements.size())
	//		return m_ShaderResourceElements[arrayIndex];
	//	return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	//}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nDsvCount = 0;
	uint32_t m_ArraySize = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_DepthArrayDSV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_DepthStencilElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

class Depth2DMSArray :public ITexture
{
public:
	Depth2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
		const DXGI_SAMPLE_DESC& sampleDesc,
		DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::DEPTH_STENCIL | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Depth2DMSArray() override = default;

	uint32_t GetArraySize() const { return m_ArraySize; }
	uint32_t GetMsaaSamples() const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
		CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize);
	void GetDescriptorCount(int& srvCount, int& dsvCount);

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() {
		if (m_nDsvCount)
			return m_DepthArrayDSV;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil(size_t arrayIndex) {
		if (arrayIndex < m_DepthStencilElements.size())
			return m_DepthStencilElements[arrayIndex];
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}
	// Texture2DMSArray
	using ITexture::GetShaderResource;
	//// Texture2DMS
	//CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource(size_t arrayIndex) {
	//	if (arrayIndex < m_ShaderResourceElements.size())
	//		return m_ShaderResourceElements[arrayIndex];
	//	return CD3DX12_GPU_DESCRIPTOR_HANDLE{};
	//}

protected:
	uint32_t m_nSrvCount = 1;
	uint32_t m_nDsvCount = 0;
	uint32_t m_ArraySize = 1;
	uint32_t m_MsaaSamples = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_DepthArrayDSV = {};
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_DepthStencilElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

#endif // !TEXTURE_H

