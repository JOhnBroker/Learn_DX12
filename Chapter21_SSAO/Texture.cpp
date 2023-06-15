#include "Texture.h"

ITexture::ITexture(ID3D12Device* device, const D3D12_RESOURCE_DESC& texDesc) 
	:m_Width(texDesc.Width), m_Height(texDesc.Height)
{
	m_Resource.Reset();
	m_UploadHeap.Reset();

	HR(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_Resource)));

	
}

void ITexture::BuildDescriptor(ID3D12Device* device, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv)
{
	m_hGpuSrv = hGpuSrv;

	device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, hCpuSrv);
}

Texture2D::Texture2D(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mipLevels, uint32_t resourceFlag) :
	ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D , 0 , width , height , 1 ,
		(UINT16)mipLevels, format, DXGI_SAMPLE_DESC {1,0}, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		(resourceFlag& (uint32_t)ResourceFlag::UNORDERED_ACCESS ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS: D3D12_RESOURCE_FLAG_NONE) |
		(resourceFlag& (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE) })
{
	m_MipLevels = m_Resource->GetDesc().MipLevels;

	if (resourceFlag & (uint32_t)ResourceFlag::UNORDERED_ACCESS) 
	{
		m_nSrvCount = 2;
	}
	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET) 
	{
		m_nRtvCount = 1;
	}
}

void Texture2D::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Resource->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = m_MipLevels;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Texture2D.PlaneSlice = 0;
	ITexture::BuildDescriptor(device, srvDesc, hCpuSrv, hGpuSrv);
	hCpuSrv.Offset(1, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

	if (m_nSrvCount > 1) 
	{
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = m_Resource->GetDesc().Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		device->CreateUnorderedAccessView(m_Resource.Get(), nullptr, &uavDesc, hCpuSrv);
	}

	if (m_nRtvCount > 0) 
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Format = m_Resource->GetDesc().Format;
		rtvDesc.Texture2D.MipSlice = 0;
		rtvDesc.Texture2D.PlaneSlice = 0;
		device->CreateRenderTargetView(m_Resource.Get(), &rtvDesc, hCpuRtv);
	}
}

void Texture2D::GetDescriptorCount(int& srvCount, int rtvCount)
{
	srvCount = m_nSrvCount;
	rtvCount = m_nRtvCount;
}

Texture2DMS::Texture2DMS(ID3D12Device* device, uint32_t width, uint32_t height,
	DXGI_FORMAT format, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t resourceFlag)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0, width, height, 1, 1, format, sampleDesc, D3D12_TEXTURE_LAYOUT_UNKNOWN,
		resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE }) 
{
	m_MsaaSamples = sampleDesc.Count;
	
	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET) 
	{
		m_nRtvCount = 1;
	}
}

void Texture2DMS::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{

}

void Texture2DMS::GetDescriptorCount(int& srvCount, int& rtvCount)
{
	srvCount = m_nSrvCount, rtvCount = m_nRtvCount;
}

TextureCube::TextureCube(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t mipLevels, uint32_t resourceFlag)
	:ITexture(device, D3D12_RESOURCE_DESC{ D3D12_RESOURCE_DIMENSION_TEXTURE2D, 
		0, width, height, 6,(UINT16)mipLevels, format, DXGI_SAMPLE_DESC {1,0}, 
		D3D12_TEXTURE_LAYOUT_UNKNOWN, resourceFlag& (uint32_t)ResourceFlag::RENDER_TARGET ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET : D3D12_RESOURCE_FLAG_NONE })
{
	m_MipLevels = mipLevels;

	m_nSrvCount = 6;
	if (resourceFlag & (uint32_t)ResourceFlag::RENDER_TARGET) 
	{
		m_nRtvCount = 6;
	}
}

void TextureCube::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
}

void TextureCube::GetDescriptorCount(int& srvCount, int& rtvCount)
{
}

Texture2DArray::Texture2DArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t arraySize, uint32_t mipLevels, uint32_t resourceFlag)
{
}

void Texture2DArray::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
}

void Texture2DArray::GetDescriptorCount(int& srvCount, int& rtvCount)
{
}

Texture2DMSArray::Texture2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, uint32_t mipLevels, uint32_t resourceFlag)
{
}

void Texture2DMSArray::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
}

void Texture2DMSArray::GetDescriptorCount(int& srvCount, int& rtvCount)
{
}

Depth2D::Depth2D(ID3D12Device* device, uint32_t width, uint32_t height, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
{
}

void Depth2D::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
}

void Depth2D::GetDescriptorCount(int& srvCount, int& dsvCount)
{
}

Depth2DMS::Depth2DMS(ID3D12Device* device, uint32_t width, uint32_t height, const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t bindFlags)
{
}

void Depth2DMS::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
}

void Depth2DMS::GetDescriptorCount(int& srvCount, int& dsvCount)
{
}

Depth2DArray::Depth2DArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
{
}

void Depth2DArray::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
}

void Depth2DArray::GetDescriptorCount(int& srvCount, int& dsvCount)
{
}

Depth2DMSArray::Depth2DMSArray(ID3D12Device* device, uint32_t width, uint32_t height, uint32_t arraySize, const DXGI_SAMPLE_DESC& sampleDesc, DepthStencilBitsFlag depthStencilBitsFlag, uint32_t resourceFlag)
{
}

void Depth2DMSArray::BuildDescriptor(ID3D12Device* device, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv)
{
}

void Depth2DMSArray::GetDescriptorCount(int& srvCount, int& dsvCount)
{
}
