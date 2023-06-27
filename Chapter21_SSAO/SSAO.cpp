#include "SSAO.h"

using namespace DirectX;
using namespace DirectX::PackedVector;

SSAO::SSAO(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, UINT width, UINT height)
{
	m_pDevice = device;
	m_RenderTargetWidth = width;
	m_RenderTargetHeight = height;

	m_ViewPort.TopLeftX = 0.0f;
	m_ViewPort.TopLeftY = 0.0f;
	m_ViewPort.Width = m_RenderTargetWidth;
	m_ViewPort.Height = m_RenderTargetHeight;
	m_ViewPort.MinDepth = 0.0f;
	m_ViewPort.MaxDepth = 1.0f;

	m_ScissotRect = { 0,0,(int)m_RenderTargetWidth,(int)m_RenderTargetHeight };

	BuildOffsetVectors();
	BuildRandomVectorTexture(cmdList);
	BuildResource();
}

void SSAO::GetOffsetVectors(DirectX::XMFLOAT4 offsets[14])
{
	memcpy_s(&offsets, sizeof(XMFLOAT4) * RANDOMVECTORCOUNT, &m_Offsets, sizeof(XMFLOAT4) * RANDOMVECTORCOUNT);
}

std::vector<float> SSAO::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= m_MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;
	for (int i = -blurRadius; i <= blurRadius; ++i) 
	{
		float x = (float)i;
		weights[i + blurRadius] = expf(-x * x / twoSigma2);
		weightSum += weights[i + blurRadius];
	}

	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void SSAO::OnResize(UINT newWidth, UINT newHeight)
{
	if (m_RenderTargetWidth != newWidth || m_RenderTargetHeight != newHeight) 
	{
		m_RenderTargetWidth = newWidth;
		m_RenderTargetHeight = newHeight;

		m_ViewPort.TopLeftX = 0.0f;
		m_ViewPort.TopLeftY = 0.0f;
		m_ViewPort.Width = m_RenderTargetWidth;
		m_ViewPort.Height = m_RenderTargetHeight;
		m_ViewPort.MinDepth = 0.0f;
		m_ViewPort.MaxDepth = 1.0f;

		m_ScissotRect = { 0,0,(int)m_RenderTargetWidth,(int)m_RenderTargetHeight };

		bIsResize = true;
		BuildResource();
	}
}

void SSAO::RenderNormalDepthMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso,
	D3D12_CPU_DESCRIPTOR_HANDLE hDssv, FrameResource* currFrame, const std::vector<RenderItem*>& items)
{
	cmdList->RSSetViewports(1, &m_ViewPort);
	cmdList->RSSetScissorRects(1, &m_ScissotRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_NormalDepthMap->GetTexture(), 
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 0.0f,0.0f,0.0f,1.0f };
	cmdList->ClearRenderTargetView(m_NormalDepthMap->GetRenderTarget(), clearValue, 0, nullptr);
	cmdList->ClearDepthStencilView(hDssv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &m_NormalDepthMap->GetRenderTarget(), true, &hDssv);

	auto passCB = currFrame->PassCB->Resource();
	auto objectCB = currFrame->ObjectCB->Resource();
	cmdList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress());

	cmdList->SetPipelineState(pPso);
	
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

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
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_NormalDepthMap->GetTexture(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));

}

void SSAO::RenderToSSAOTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso,
	FrameResource* currFrame)
{
	cmdList->RSSetViewports(1, &m_ViewPort);
	cmdList->RSSetScissorRects(1, &m_ScissotRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOMap0->GetTexture(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f,1.0f,1.0f,1.0f };
	cmdList->ClearRenderTargetView(m_SSAOMap0->GetRenderTarget(), clearValue, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &m_SSAOMap0->GetRenderTarget(), true, nullptr);

	auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
	cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

	cmdList->SetGraphicsRootDescriptorTable(1, m_NormalDepthMap->GetShaderResource());
	cmdList->SetGraphicsRootDescriptorTable(2, m_RandomVectorMap->GetShaderResource());
	cmdList->SetGraphicsRootDescriptorTable(3, m_SSAOMap0->GetShaderResource());
	
	cmdList->SetPipelineState(pPso);

	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAOMap0->GetTexture(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAO::BlurAOMap(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPsoX, ID3D12PipelineState* pPsoY,
	FrameResource* currFrame, int blurCount)
{
	auto ssaoCBAddress = currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
	cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

	cmdList->SetPipelineState(pPsoX);
	for (int i = 0; i < blurCount; ++i) 
	{
		BlurAOMap(cmdList, true);
	}
	cmdList->SetPipelineState(pPsoY);
	for (int i = 0; i < blurCount; ++i) 
	{
		BlurAOMap(cmdList, false);
	}
}

void SSAO::RenderAOToTexture(ID3D12GraphicsCommandList* cmdList, ID3D12PipelineState* pPso)
{
	cmdList->RSSetViewports(1, &m_ViewPort);
	cmdList->RSSetScissorRects(1, &m_ScissotRect);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAODebugMap->GetTexture(), 
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	cmdList->OMSetRenderTargets(1, &m_SSAODebugMap->GetRenderTarget(), true, nullptr);

	cmdList->SetPipelineState(pPso);

	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);
	
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_SSAODebugMap->GetTexture(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAO::BuildResource()
{
	TextureManager& textureManager = TextureManager::Get();
	if (bIsResize) 
	{
		m_NormalDepthMap = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_NormalDepthMapFormat);
		m_SSAOMap0 = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_AOMapFormat);
		m_SSAOMap1 = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_AOMapFormat);
		m_SSAODebugMap = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		textureManager.ReBuildDescriptor(NORMALDEPTHMAP_NAME, m_NormalDepthMap);
		textureManager.ReBuildDescriptor(SSAOXMAP_NAME, m_SSAOMap0);
		textureManager.ReBuildDescriptor(SSAOYMAP_NAME, m_SSAOMap1);
		textureManager.ReBuildDescriptor(SSAODEBUGMAP_NAME, m_SSAODebugMap);
	}
	else 
	{
		m_RandomVectorMap = std::make_shared<Texture2D>(m_pDevice, 256, 256,
			DXGI_FORMAT_R8G8B8A8_UNORM, 1, static_cast<uint32_t>(ResourceFlag::SHADER_RESOURCE), 
			m_RandomVectorResource, m_RandomVectorUploadBuffer); 
		m_NormalDepthMap = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_NormalDepthMapFormat);
		m_SSAOMap0 = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_AOMapFormat);
		m_SSAOMap1 = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, m_AOMapFormat);
		m_SSAODebugMap = std::make_shared<Texture2D>(m_pDevice, m_RenderTargetWidth,
			m_RenderTargetHeight, DXGI_FORMAT_R8G8B8A8_UNORM);
		textureManager.AddTexture(NORMALDEPTHMAP_NAME, m_NormalDepthMap);
		textureManager.AddTexture(RANDOMVECTORMAP_NAME, m_RandomVectorMap);
		textureManager.AddTexture(SSAOXMAP_NAME, m_SSAOMap0);
		textureManager.AddTexture(SSAOYMAP_NAME, m_SSAOMap1);
		textureManager.AddTexture(SSAODEBUGMAP_NAME, m_SSAODebugMap);
	}
}

void SSAO::BuildRandomVectorTexture(ID3D12GraphicsCommandList* cmdList)
{
	int width = 256;
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = width;
	texDesc.Height = width;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	HR(m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_RandomVectorResource)));

    const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_RandomVectorResource.Get(), 0, num2DSubresources);

	HR(m_pDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_RandomVectorUploadBuffer.GetAddressOf())));

	XMCOLOR initData[256 * 256];
	for (int i = 0; i < width; ++i)
	{
		for (int j = 0; j < width; ++j)
		{
			XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());
			initData[i * width + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
		}
	}

	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData;
	subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
	subResourceData.SlicePitch = subResourceData.RowPitch * 256;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorResource.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST));
	// 将CPU数据复制到GPU
	UpdateSubresources(cmdList, m_RandomVectorResource.Get(), m_RandomVectorUploadBuffer.Get(),
		0, 0, num2DSubresources, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorResource.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
}

void SSAO::BuildOffsetVectors()
{
	// 8 cube corners
	m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
	m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);

	m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);

	m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
	m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);

	// 6 centers of cube faces
	m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
	m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);

	m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
	m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);

	m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
	m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

	for (int i = 0; i < 14; ++i)
	{
		// Create random lengths in [0.25, 1.0].
		float s = MathHelper::RandF(0.25f, 1.0f);

		XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

		XMStoreFloat4(&m_Offsets[i], v);
	}
}

void SSAO::BlurAOMap(ID3D12GraphicsCommandList* cmdList, bool horzBlur)
{
	ID3D12Resource* output = nullptr;
	CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

	if (true == horzBlur) 
	{
		output = m_SSAOMap1->GetTexture();
		inputSrv = m_SSAOMap0->GetShaderResource();
		outputRtv = m_SSAOMap1->GetRenderTarget();
	}
	else 
	{
		output = m_SSAOMap0->GetTexture();
		inputSrv = m_SSAOMap1->GetShaderResource();
		outputRtv = m_SSAOMap0->GetRenderTarget();
	}
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET));

	float clearValue[] = { 1.0f,1.0f,1.0f,1.0f };
	cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);
	cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

	cmdList->SetGraphicsRootDescriptorTable(1, m_NormalDepthMap->GetShaderResource());
	cmdList->SetGraphicsRootDescriptorTable(2, m_RandomVectorMap->GetShaderResource());
	cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);

	cmdList->IASetVertexBuffers(0, 0, nullptr);
	cmdList->IASetIndexBuffer(nullptr);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	cmdList->DrawInstanced(3, 1, 0, 0);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(output,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ));
}
