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

enum class TextureType
{
	Texture2D = 0,
	Texture2DMS,
	TextureCube,
	Texture2DArray,
	Texture2DMSArray,
	Depth2D,
	Depth2DMS,
	Depth2DArray,
	Depth2DMSArray,
	UNKNOWN
};

class ITexture 
{
public:
	template<class T>
	using ComPtr = Microsoft::WRL::ComPtr<T>;

	ITexture(ID3D12Device* device, const D3D12_RESOURCE_DESC& texDesc,
		ComPtr<ID3D12Resource> resource = nullptr,
		ComPtr<ID3D12Resource> uploadHead = nullptr,
		D3D12_CLEAR_VALUE clearValue = {});
	virtual ~ITexture() = 0 {};

	ID3D12Resource* GetTexture() { return m_Resource.Get(); }
	ID3D12Resource* GetUploadHeap() { return m_UploadHeap.Get(); }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetShaderResource() { return m_hGpuSrv; }
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetUnorderedAccess() { return m_hGpuUav; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() { return m_hCpuRtv; }
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() { return m_hCpuDsv; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	TextureType GetTextureType() { return m_TextureType; } 

	virtual void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) = 0;

	UINT GetSRVDescriptorCount() { return m_nSrvCount; };
	UINT GetRTVDescriptorCount() { return m_nRtvCount; };
	UINT GetDSVDescriptorCount() { return m_nDsvCount; };

protected:
	ComPtr<ID3D12Resource> m_Resource = nullptr;
	ComPtr<ID3D12Resource> m_UploadHeap = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuSrv = {};
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_hGpuUav = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuRtv = {};
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_hCpuDsv = {};
	UINT m_nSrvCount = 1;
	UINT m_nRtvCount = 0;
	UINT m_nDsvCount = 0;

	uint32_t m_Width = 0, m_Height = 0;
	TextureType m_TextureType = TextureType::UNKNOWN;
};

class Texture2D :public ITexture 
{
public:
	Texture2D(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t mipLevels = 1, 
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE, 
		ComPtr<ID3D12Resource> resource = nullptr, ComPtr<ID3D12Resource> uploadHead = nullptr); 
	~Texture2D()override = default;

	// 不允许拷贝，允许移动
	Texture2D(const Texture2D&) = delete;
	Texture2D& operator=(Texture2D&) = delete;
	Texture2D(Texture2D&&) = default;
	Texture2D& operator=(Texture2D&&) = default;

	uint32_t GetMiopLevels() const { return m_MipLevels; };

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

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
	uint32_t m_MipLevels = 1;
};

class Texture2DMS :public ITexture 
{
public:
	Texture2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
		DXGI_FORMAT format, const DXGI_SAMPLE_DESC& sampleDesc,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Texture2DMS()override = default;

	uint32_t GetMsaaSamples()const { return m_MsaaSamples; }

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() 
	{
		if (m_nRtvCount)
			return m_hCpuRtv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

protected:
	uint32_t m_MsaaSamples = 1;
};

class TextureCube :public ITexture 
{
public:
	TextureCube(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format,
		uint32_t mipLevels = 1,
		uint32_t resourceFlag = (uint32_t)ResourceFlag::RENDER_TARGET | (uint32_t)ResourceFlag::SHADER_RESOURCE,
		ComPtr<ID3D12Resource> resource = nullptr, ComPtr<ID3D12Resource> uploadHead = nullptr);
	~TextureCube() override = default;

	uint32_t GetMipLevels() const { return m_MipLevels; }

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount) 
			return m_hCpuRtv;
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
	uint32_t m_MipLevels = 1;
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
	
	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount)
			return m_hCpuRtv;
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
	uint32_t m_MipLevels = 1;
	uint32_t m_ArraySize = 1;
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

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRenderTarget() {
		if (m_nRtvCount)
			return m_hCpuRtv;
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
	uint32_t m_ArraySize = 1;
	uint32_t m_MsaaSamples = 1;
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

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() 
	{
		if (m_nDsvCount)
			return m_hCpuDsv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

protected:
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
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

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil()
	{
		if (m_nDsvCount)
			return m_hCpuDsv;
		return CD3DX12_CPU_DESCRIPTOR_HANDLE{};
	}

protected:
	uint32_t m_MsaaSamples = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
};

class Depth2DArray : public ITexture
{
public:
	Depth2DArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
		DepthStencilBitsFlag depthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits, 
		uint32_t resourceFlag = (uint32_t)ResourceFlag::DEPTH_STENCIL | (uint32_t)ResourceFlag::SHADER_RESOURCE);
	~Depth2DArray() override = default;

	uint32_t GetArraySize() const { return m_ArraySize; }

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() {
		if (m_nDsvCount)
			return m_hCpuDsv;
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
	uint32_t m_ArraySize = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
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

	void BuildDescriptor(ID3D12Device* device,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE& hGpuSrv,
		CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE& hCpuRtv,
		UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize) override;

	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDepthStencil() {
		if (m_nDsvCount)
			return m_hCpuDsv;
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
	uint32_t m_ArraySize = 1;
	uint32_t m_MsaaSamples = 1;
	DepthStencilBitsFlag m_DepthStencilBitsFlag = DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits;
	std::vector<CD3DX12_CPU_DESCRIPTOR_HANDLE> m_DepthStencilElements = {};
	//std::vector<CD3DX12_GPU_DESCRIPTOR_HANDLE> m_ShaderResourceElements = {};
};

#endif // !TEXTURE_H

