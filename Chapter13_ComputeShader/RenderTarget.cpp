#include "RenderTarget.h"

RenderTarget::RenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
{
	m_d3dDevice = device;
	m_uiWidth = width;
	m_uiHeight = height;
	m_Format = format;

	BuildResource();
}

ID3D12Resource* RenderTarget::Resource()
{
	return m_OffscreenTex.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE RenderTarget::GetSrv()
{
	return m_hGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE RenderTarget::GetRtv()
{
	return m_hCpuRtv;
}

void RenderTarget::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuUav, CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv)
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuUav;
	m_hCpuRtv = hCpuRtv;

	BuildDescriptors();
}

void RenderTarget::OnResize(UINT newWidth, UINT newHeight)
{
	if ((m_uiWidth != newWidth) || (m_uiHeight != newHeight))
	{
		m_uiWidth = newWidth;
		m_uiHeight = newHeight;

		BuildResource();
		BuildDescriptors();
	}
}

void RenderTarget::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	m_d3dDevice->CreateShaderResourceView(m_OffscreenTex.Get(), &srvDesc, m_hCpuSrv);
	m_d3dDevice->CreateRenderTargetView(m_OffscreenTex.Get(), nullptr, m_hCpuRtv);
}

void RenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_uiWidth;
	texDesc.Height = m_uiHeight;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	D3D12_CLEAR_VALUE optimizedClearValue = {};
	optimizedClearValue.Format = m_Format;
	float fogColor[4] = { 0.7f,0.7f,0.7f,1.0f };
	memcpy(optimizedClearValue.Color, fogColor, sizeof(float) * 4);

	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		&optimizedClearValue,
		IID_PPV_ARGS(&m_OffscreenTex)));
}
