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

