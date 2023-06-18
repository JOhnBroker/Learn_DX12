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
	return m_ShadowMap->GetTexture().Get();
}

ID3D12Resource* ShadowMap::GetDebugResource()
{
	return m_ShadowMap_Debug->GetTexture().Get();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetSrv()const
{
	return m_ShadowMap->GetShaderResource();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDsv()const
{
	return m_ShadowMap->GetDepthStencil();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE ShadowMap::GetDebugRtv() const
{
	return m_ShadowMap_Debug->GetRenderTarget();
}

CD3DX12_GPU_DESCRIPTOR_HANDLE ShadowMap::GetDebugSrv() const
{
	return m_ShadowMap_Debug->GetShaderResource();
}

D3D12_VIEWPORT ShadowMap::GetViewport()const
{
	return m_Viewport;
}

D3D12_RECT ShadowMap::GetScissorRect()const
{
	return m_ScissorRect;
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
	}
}

void ShadowMap::BuildResource()
{
	TextureManager& textureManager = TextureManager::Get();

	m_ShadowMap = std::make_shared<Depth2D>(m_pd3dDevice, m_Width, m_Height);
	m_ShadowMap_Debug = std::make_shared<Texture2D>(m_pd3dDevice, m_Width, m_Height,
		DXGI_FORMAT_R8G8B8A8_UNORM);

	if (bIsResize) 
	{
		// TODO ：可能和思考的情况，不相符
		ITexture* ori = textureManager.GetTexture(SHADOWMAP_NAME);
		ori = std::move(m_ShadowMap.get());
		ori = textureManager.GetTexture(SHADOWMAP_DEBUG_NAME);
		ori = std::move(m_ShadowMap_Debug.get());
	}
	else 
	{
		textureManager.AddTexture(SHADOWMAP_NAME, m_ShadowMap);
		textureManager.AddTexture(SHADOWMAP_DEBUG_NAME, m_ShadowMap_Debug);
	}
}