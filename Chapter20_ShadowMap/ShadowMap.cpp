//***************************************************************************************
// ShadowMap.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "ShadowMap.h"
 
ShadowMap::ShadowMap(ID3D12Device* device, UINT width, UINT height)
{
	m_pd3dDevice = device;

	m_Width = width;
	m_Height = height;

	m_Viewport = { 0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f };
	m_ScissorRect = { 0, 0, (int)width, (int)height };

	BuildResource();
}

UINT ShadowMap::GetWidth()const
{
    return m_Width;
}

UINT ShadowMap::GetHeight()const
{
    return m_Height;
}

ID3D12Resource*  ShadowMap::GetResource()
{
	return m_ShadowMap.Get();
}

ID3D12Resource* ShadowMap::GetDebugResource()
{
	return m_DebugShadowMap.Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSrv()const
{
	return m_hGpuSrv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDsv()const
{
	return m_hCpuDsv;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDebugRtv() const
{
	return m_hCpuRtv_Debug;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetDebugSrv() const
{
	return m_hGpuSrv_Debug;
}

D3D12_VIEWPORT ShadowMap::GetViewport()const
{
	return m_Viewport;
}

D3D12_RECT ShadowMap::GetScissorRect()const
{
	return m_ScissorRect;
}

void ShadowMap::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv,
	                             CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv,
	                             CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDsv,
								 CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuRtv_Debug,
								 CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuSrv_Debug,
								 CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuSrv_Debug)
{
	m_hCpuSrv = hCpuSrv;
	m_hGpuSrv = hGpuSrv;
    m_hCpuDsv = hCpuDsv;
	m_hCpuRtv_Debug = hCpuRtv_Debug;
	m_hCpuSrv_Debug = hCpuSrv_Debug;
	m_hGpuSrv_Debug = hGpuSrv_Debug;

	//  Create the descriptors
	BuildDescriptors();
}

void ShadowMap::OnResize(UINT newWidth, UINT newHeight)
{
	if((m_Width != newWidth) || (m_Height != newHeight))
	{
		m_Width = newWidth;
		m_Height = newHeight;

		m_Viewport = { 0.0f, 0.0f, (float)newWidth, (float)newHeight, 0.0f, 1.0f };
		m_ScissorRect = { 0, 0, (int)newWidth, (int)newHeight };

		BuildResource();
		BuildDescriptors();
	}
}
 
void ShadowMap::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; 
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
    m_pd3dDevice->CreateShaderResourceView(m_ShadowMap.Get(), &srvDesc, m_hCpuSrv);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc; 
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.Texture2D.MipSlice = 0;
	m_pd3dDevice->CreateDepthStencilView(m_ShadowMap.Get(), &dsvDesc, m_hCpuDsv);

	// Debug
	D3D12_SHADER_RESOURCE_VIEW_DESC debugSrvDesc = {};
	debugSrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	debugSrvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	debugSrvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	debugSrvDesc.Texture2D.MostDetailedMip = 0;
	debugSrvDesc.Texture2D.MipLevels = 1;
	debugSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	debugSrvDesc.Texture2D.PlaneSlice = 0;
	m_pd3dDevice->CreateShaderResourceView(m_DebugShadowMap.Get(), &debugSrvDesc, m_hCpuSrv_Debug);

	D3D12_RENDER_TARGET_VIEW_DESC debugRtvDesc;
	debugRtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	debugRtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	debugRtvDesc.Texture2D.MipSlice = 0;
	debugRtvDesc.Texture2D.PlaneSlice = 0;
	m_pd3dDevice->CreateRenderTargetView(m_DebugShadowMap.Get(), &debugRtvDesc, m_hCpuRtv_Debug);
}

void ShadowMap::BuildResource()
{
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
		&optClear,
		IID_PPV_ARGS(&m_ShadowMap)));

	//m_DebugShadowMap
	D3D12_RESOURCE_DESC debugDesc;
	ZeroMemory(&debugDesc, sizeof(D3D12_RESOURCE_DESC));
	debugDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	debugDesc.Alignment = 0;
	debugDesc.Width = m_Width;
	debugDesc.Height = m_Height;
	debugDesc.DepthOrArraySize = 6;
	debugDesc.MipLevels = 1;
	debugDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	debugDesc.SampleDesc.Count = 1;
	debugDesc.SampleDesc.Quality = 0;
	debugDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	debugDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&debugDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_DebugShadowMap)));
}