#include "Texture.h"

ITexture::ITexture(ID3D12Device* device, const D3D12_RESOURCE_DESC& texDesc,
	ComPtr<ID3D12Resource> resource, ComPtr<ID3D12Resource> uploadHead) 
	:m_Width(texDesc.Width), m_Height(texDesc.Height)
{
	m_Resource.Reset();
	m_UploadHeap.Reset();
	if (resource != nullptr && m_UploadHeap != nullptr) 
	{
		m_Resource = std::move(resource);
		m_UploadHeap = std::move(uploadHead);
	}
	else 
	{
		// TODO : 区分创建资源的用途 还有D3D12_HEAP_TYPE_UPLOAD、D3D12_HEAP_TYPE_READBACK
		HR(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&m_Resource)));
	}
}

Texture2D::Texture2D(ID3D12Device* device, uint32_t width, uint32_t height,
	DXGI_FORMAT format, uint32_t mipLevels, uint32_t resourceFlag,
	ComPtr<ID3D12Resource> resource, ComPtr<ID3D12Resource> uploadHead) :
	ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D , 0 , width , height , 1 ,
		(UINT16)mipLevels, format, DXGI_SAMPLE_DESC {1,0}, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		(resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE) |
		(resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE) },
		resource, uploadHead) 
{
	m_TextureType = TextureType::Texture2D;
	m_MipLevels = m_Resource->GetDesc().MipLevels;

	if (resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS)
	{
		m_nSrvCount += 1;
	}
	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET)
	{
		m_nRtvCount += 1;
	}
}

void Texture2D::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);

	if (m_nSrvCount > 1)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = m_Resource->GetDesc().Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, hCpuSrv);
		m_hGpuUav = hGpuSrv;
	}

	if (m_nRtvCount > 0)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
		m_hCpuRtv = hCpuRtv;
	}
}

Texture2DMS::Texture2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
	DXGI_FORMAT format, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t resourceFlag)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0, width, height, 1, 1, format, sampleDesc, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE }) 
{
	m_TextureType = TextureType::Texture2DMS;
	m_MsaaSamples = sampleDesc.Count;
	
	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET) 
	{
		m_nRtvCount += 1;
	}
}

void Texture2DMS::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);

	if (m_nRtvCount > 0)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
		m_hCpuRtv = hCpuRtv;
	}
}

TextureCube::TextureCube(ID3D12Device* device, uint32_t width, uint32_t height,
	DXGI_FORMAT format, uint32_t mipLevels, uint32_t resourceFlag,
	ComPtr<ID3D12Resource> resource, ComPtr<ID3D12Resource> uploadHead)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			0, width, height, 6, (UINT16)mipLevels, format, DXGI_SAMPLE_DESC {1,0},
			D3D12_TEXTURE_LAYOUT_UNKNOWN, (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE) |
			(resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE) },
		resource, uploadHead) 
{
	m_TextureType = TextureType::TextureCube;
	m_MipLevels = mipLevels;

	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET)
	{
		m_nRtvCount += 6;
	}
	if (resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS)
	{
		m_nSrvCount += 6;
	}
}

void TextureCube::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = m_MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);

	for (UINT i = 0; i < m_nRtvCount; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
		m_RenderTargetElements[i] = hCpuRtv;
		hCpuRtv.Offset(1, uiRtvDescriptorSize);
	}
	if (m_nRtvCount) 
		m_hCpuRtv = m_RenderTargetElements[0];
	for (UINT i = 1; i < m_nSrvCount; ++i)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Format = m_Resource->GetDesc().Format;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.PlaneSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = i;
		uavDesc.Texture2DArray.ArraySize = 1;
		device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, hCpuSrv);
		m_UnorderedAccessElements[i] = hGpuSrv;
		hCpuSrv.Offset(1, uiSrvDescriptorSize);
		hGpuSrv.Offset(1, uiSrvDescriptorSize);
	}
}

Texture2DArray::Texture2DArray(ID3D12Device* device, uint32_t width, uint32_t height,
	DXGI_FORMAT format, uint32_t arraySize, uint32_t mipLevels, uint32_t resourceFlag)
	: ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, width, height,
		(UINT16)arraySize, (UINT16)mipLevels, format, DXGI_SAMPLE_DESC{1,0}, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		(resourceFlag& (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE) | 
		(resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS : D3D12_RESOURCE_FLAG_NONE)})
{
	m_TextureType = TextureType::Texture2DArray;
	m_MipLevels = m_Resource->GetDesc().MipLevels;
	m_ArraySize = arraySize;

	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET)
	{
		m_nRtvCount += arraySize;
	}
	if (resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS)
	{
		m_nSrvCount += arraySize;
	}

}

void Texture2DArray::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = m_MipLevels;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = m_ArraySize;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);

	for (UINT i = 0; i < m_nRtvCount; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;
		rtvDesc.Texture2DArray.ArraySize = 1;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
		m_RenderTargetElements[i] = hCpuRtv;
		hCpuRtv.Offset(1, uiRtvDescriptorSize);
	}
	if (m_nRtvCount)
		m_hCpuRtv = m_RenderTargetElements[0];
	for (UINT i = 1; i < m_nSrvCount; ++i)
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		uavDesc.Format = m_Resource->GetDesc().Format;
		uavDesc.Texture2DArray.MipSlice = 0;
		uavDesc.Texture2DArray.PlaneSlice = 0;
		uavDesc.Texture2DArray.FirstArraySlice = i;
		uavDesc.Texture2DArray.ArraySize = 1;
		device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, hCpuSrv);
		m_UnorderedAccessElements[i] = hGpuSrv;
		hCpuSrv.Offset(1, uiSrvDescriptorSize);
		hGpuSrv.Offset(1, uiSrvDescriptorSize);
	}
}

Texture2DMSArray::Texture2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, 
	uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t resourceFlag)
	: ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, width, height,
		(UINT16)arraySize, 1, format, DXGI_SAMPLE_DESC{1,0}, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		(resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE)	})
{
	m_TextureType = TextureType::Texture2DMSArray;
	m_ArraySize = arraySize;
	m_MsaaSamples = sampleDesc.Count;

	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET) 
	{
		m_nRtvCount += arraySize;
	}
}

void Texture2DMSArray::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
	srvDesc.Texture2DMSArray.FirstArraySlice = 0;
	srvDesc.Texture2DMSArray.ArraySize = m_ArraySize;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	hCpuSrv.Offset(1, uiSrvDescriptorSize);
	hGpuSrv.Offset(1, uiSrvDescriptorSize);

	for (UINT i = 0; i < m_nRtvCount; ++i)
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2DMSArray.FirstArraySlice = i;
		rtvDesc.Texture2DMSArray.ArraySize = 1;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
		m_RenderTargetElements[i] = hCpuRtv;
		hCpuRtv.Offset(1, uiRtvDescriptorSize);
	}
	if (m_nRtvCount)
		m_hCpuRtv = m_RenderTargetElements[0];
}

static DXGI_FORMAT GetDepthTextureFormat(DepthStencilBitsFlag flag)
{
	switch (flag)
	{
	case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_R16_TYPELESS;
	case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_R24G8_TYPELESS;
	case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_R32_TYPELESS;
	case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_R32G8X24_TYPELESS;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

static DXGI_FORMAT GetDepthSRVFormat(DepthStencilBitsFlag flag)
{
	switch (flag)
	{
	case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_R16_FLOAT;
	case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_R32_FLOAT;
	case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

static DXGI_FORMAT GetDepthDSVFormat(DepthStencilBitsFlag flag)
{
	switch (flag)
	{
	case DepthStencilBitsFlag::Depth_16Bits: return DXGI_FORMAT_D16_UNORM;
	case DepthStencilBitsFlag::Depth_24Bits_Stencil_8Bits: return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case DepthStencilBitsFlag::Depth_32Bits: return DXGI_FORMAT_D32_FLOAT;
	case DepthStencilBitsFlag::Depth_32Bits_Stencil_8Bits_Unused_24Bits: return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	default: return DXGI_FORMAT_UNKNOWN;
	}
}

Depth2D::Depth2D(ID3D12Device* device, uint32_t width, uint32_t height,
	DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D , 0, width, height, 1, 1, 
		GetDepthTextureFormat(depthStencilBitsFlag), DXGI_SAMPLE_DESC{1,0},D3D12_TEXTURE_LAYOUT_UNKNOWN,
		resourceFlag& (uint32_t)ResourceFlag::DEPTH_STENCIL ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE })
{
	m_TextureType = TextureType::Depth2D;
	m_DepthStencilBitsFlag = depthStencilBitsFlag;
	if (resourceFlag & (uint32_t)ResourceFlag::DEPTH_STENCIL)
	{
		m_nDsvCount += 1;
	}
}

void Depth2D::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = GetDepthSRVFormat(m_DepthStencilBitsFlag);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;
	
	for (UINT i = 0; i < m_nDsvCount; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Format = GetDepthDSVFormat(m_DepthStencilBitsFlag);
		dsvDesc.Texture2D.MipSlice = 0;
		device->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, hCpuDsv);
		m_hCpuDsv = hCpuDsv;
	}
}

Depth2DMS::Depth2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
	const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D , 0, width, height,
		1, 1, GetDepthTextureFormat(depthStencilBitsFlag), sampleDesc, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		resourceFlag& (uint32_t)ResourceFlag::DEPTH_STENCIL ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE })
{
	m_TextureType = TextureType::Depth2DMS;
	m_MsaaSamples = sampleDesc.Count;
	m_DepthStencilBitsFlag = depthStencilBitsFlag;

	if (resourceFlag & (uint32_t)ResourceFlag::DEPTH_STENCIL)
	{
		m_nDsvCount += 1;
	}
}

void Depth2DMS::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = GetDepthSRVFormat(m_DepthStencilBitsFlag);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	srvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;

	for (UINT i = 0; i < m_nDsvCount; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
		dsvDesc.Format = GetDepthDSVFormat(m_DepthStencilBitsFlag);
		dsvDesc.Texture2DMS.UnusedField_NothingToDefine = 0;
		device->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, hCpuDsv);
		m_hCpuDsv = hCpuDsv;
	}
}

Depth2DArray::Depth2DArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
	DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
	: ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, width, height,
		(UINT16)arraySize, 1, GetDepthTextureFormat(depthStencilBitsFlag), DXGI_SAMPLE_DESC{1,0}, D3D12_TEXTURE_LAYOUT_UNKNOWN, 
		resourceFlag& (uint32_t)ResourceFlag::DEPTH_STENCIL ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE })
{
	m_TextureType = TextureType::Depth2DArray;
	m_ArraySize = arraySize;
	m_DepthStencilBitsFlag = depthStencilBitsFlag;

	if (resourceFlag & (uint32_t)ResourceFlag::DEPTH_STENCIL) 
	{
		m_nDsvCount += m_ArraySize;
	}
}

void Depth2DArray::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = GetDepthSRVFormat(m_DepthStencilBitsFlag);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	srvDesc.Texture2DArray.MostDetailedMip = 0;
	srvDesc.Texture2DArray.MipLevels = -1;
	srvDesc.Texture2DArray.FirstArraySlice = 0;
	srvDesc.Texture2DArray.ArraySize = m_ArraySize;
	srvDesc.Texture2DArray.PlaneSlice = 0;
	srvDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;

	for (UINT i = 0; i < m_nDsvCount; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
		dsvDesc.Format = GetDepthDSVFormat(m_DepthStencilBitsFlag);
		dsvDesc.Texture2DArray.MipSlice = 0;
		dsvDesc.Texture2DArray.FirstArraySlice = i;
		dsvDesc.Texture2DArray.ArraySize = 0;
		device->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, hCpuDsv);
		m_DepthStencilElements[i] = hCpuDsv;
		hCpuDsv.Offset(1, uiDsvDescriptorSize);
	}
	m_hCpuDsv = m_DepthStencilElements[0];
}

Depth2DMSArray::Depth2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize,
	const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
	: ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 0, width, height,
		(UINT16)arraySize, 1, GetDepthTextureFormat(depthStencilBitsFlag), sampleDesc, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		resourceFlag& (uint32_t)ResourceFlag::DEPTH_STENCIL ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_NONE })
{
	m_TextureType = TextureType::Depth2DMSArray;
	m_ArraySize = arraySize;
	m_MsaaSamples = sampleDesc.Count;
	m_DepthStencilBitsFlag = depthStencilBitsFlag;

	if (resourceFlag & (uint32_t)ResourceFlag::DEPTH_STENCIL) 
	{
		m_nDsvCount += m_ArraySize;
	}
}

void Depth2DMSArray::BuildDescriptor(ID3D12Device* device,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv,
	UINT uiSrvDescriptorSize, UINT uiDsvDescriptorSize, UINT uiRtvDescriptorSize)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = GetDepthSRVFormat(m_DepthStencilBitsFlag);
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
	srvDesc.Texture2DMSArray.FirstArraySlice = 0;
	srvDesc.Texture2DMSArray.ArraySize = m_ArraySize;
	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
	m_hGpuSrv = hGpuSrv;

	for (UINT i = 0; i < m_nDsvCount; ++i)
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
		dsvDesc.Format = GetDepthDSVFormat(m_DepthStencilBitsFlag);
		dsvDesc.Texture2DMSArray.FirstArraySlice = i;
		dsvDesc.Texture2DMSArray.ArraySize = m_ArraySize;
		device->CreateDepthStencilView(m_Resource.Get(), &dsvDesc, hCpuDsv);
		m_DepthStencilElements[i] = hCpuDsv;
		hCpuDsv.Offset(1, uiDsvDescriptorSize);
	}
	m_hCpuDsv = m_DepthStencilElements[0];
}