#include "CubeRenderTarget.h"

CubeRenderTarget::CubeRenderTarget(ID3D12Device* device, UINT width, UINT height, DXGI_FORMAT format)
	:m_pd3dDevice(device), m_Width(width), m_Height(height), m_Format(format)
{
	m_Viewport = { 0.0f,0.0f,(float)width,(float)height,0.0f,1.0f };
	m_ScissorRect = { 0,0,(int)width,(int)height };
	BuildResource();
}

ID3D12Resource* CubeRenderTarget::Resource()
{
	return m_CubeMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE CubeRenderTarget::GetSrv()
{
	return m_hGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE CubeRenderTarget::GetRtv(int faceIndex)
{
	return m_hCpuRtv[faceIndex];
}

D3D12_VIEWPORT CubeRenderTarget::Viewport() const
{
	return m_Viewport;
}

D3D12_RECT CubeRenderTarget::ScissorRect() const
{
	return m_ScissorRect;
}

void CubeRenderTarget::BuildDescriptors(
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv[6])
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;

	for (int i = 0; i < 6; ++i) 
	{
		m_hCpuRtv[i] = hCpuRtv[i];
	}
	BuildDescriptors();
}

void CubeRenderTarget::Resize(UINT newWidth, UINT newHeight)
{
	if ((m_Width != newWidth) || (m_Height != newHeight)) 
	{
		m_Width = newWidth;
		m_Height = newHeight;

		BuildResource();
		BuildDescriptors();
	}
}

void CubeRenderTarget::BuildDescriptors()
{
	//SRV
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.MipLevels = 1;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

	m_pd3dDevice->CreateShaderResourceView(m_CubeMap.Get(), &srvDesc, m_hCpuSrv);

	//RTV
	for (int i = 0; i < 6; ++i) 
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		rtvDesc.Format = m_Format;
		rtvDesc.Texture2DArray.MipSlice = 0;
		rtvDesc.Texture2DArray.PlaneSlice = 0;
		rtvDesc.Texture2DArray.FirstArraySlice = i;	// 用于纹理数组的第一个纹理的索引
		rtvDesc.Texture2DArray.ArraySize = 1;		// 数组中的纹理数量
		m_pd3dDevice->CreateRenderTargetView(m_CubeMap.Get(), &rtvDesc, m_hCpuRtv[i]);
	}
}

void CubeRenderTarget::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 6;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_CubeMap)));

}
