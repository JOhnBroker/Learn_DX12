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
	return m_ShadowMap->GetTexture();
}

ID3D12Resource* ShadowMap::GetDebugResource()
{
	return m_ShadowMap_Debug->GetTexture();
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

		bIsResize = true;

		BuildResource();

		bIsResize = false;
	}
}

void ShadowMap::DrawShadowMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso,
	FrameResource* currFrame, const std::vector<RenderItem*>& items)
{
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	cmdList->RSSetViewports(1, &m_Viewport);
	cmdList->RSSetScissorRects(1, &m_ScissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->GetTexture(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));


	cmdList->ClearDepthStencilView(m_ShadowMap->GetDepthStencil(),
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	cmdList->OMSetRenderTargets(0, nullptr, false, &m_ShadowMap->GetDepthStencil());

	auto passCB = currFrame->PassCB->Resource();
	auto objectCB = currFrame->ObjectCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + 1 * passCBByteSize;
	cmdList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	cmdList->SetPipelineState(pPso);

	for (size_t i = 0; i < items.size(); ++i)
	{
		auto ri = items[i];

		cmdList->IASetVertexBuffers(0, 1, &ri->Geo->GetVertexBufferView());
		cmdList->IASetIndexBuffer(&ri->Geo->GetIndexBufferView());
		cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
		cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap->GetTexture(),
		D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void ShadowMap::RenderShadowMapToTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso)
{
	cmdList->RSSetViewports(1, &m_Viewport);
	cmdList->RSSetScissorRects(1, &m_ScissorRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap_Debug->GetTexture(), 
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->OMSetRenderTargets(1, &m_ShadowMap_Debug->GetRenderTarget(), true, nullptr);

	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	cmdList->SetPipelineState(pPso);
	cmdList->DrawInstanced(3, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_ShadowMap_Debug->GetTexture(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void ShadowMap::BuildResource()
{
	TextureManager& textureManager = TextureManager::Get();

	if (bIsResize) 
	{
		m_ShadowMap = std::make_shared<Depth2D>(m_pd3dDevice, m_Width, m_Height);
		textureManager.ReBuildDescriptor(SHADOWMAP_NAME, m_ShadowMap);

		m_ShadowMap_Debug = std::make_shared<Texture2D>(m_pd3dDevice, m_Width, m_Height,
			DXGI_FORMAT_R8G8B8A8_UNORM);
		textureManager.ReBuildDescriptor(SHADOWMAP_DEBUG_NAME, m_ShadowMap_Debug);
	}
	else 
	{
		m_ShadowMap = std::make_shared<Depth2D>(m_pd3dDevice, m_Width, m_Height);
		m_ShadowMap_Debug = std::make_shared<Texture2D>(m_pd3dDevice, m_Width, m_Height,
			DXGI_FORMAT_R8G8B8A8_UNORM);
		textureManager.AddTexture(SHADOWMAP_NAME, m_ShadowMap);
		textureManager.AddTexture(SHADOWMAP_DEBUG_NAME, m_ShadowMap_Debug);
	}
}