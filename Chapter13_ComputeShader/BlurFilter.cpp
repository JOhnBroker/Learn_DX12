#include "BlurFilter.h"

BlurFilter::BlurFilter(ID3D12Device* device, UINT uiWidth, UINT uiHeight, DXGI_FORMAT format)
{
	m_d3dDevice = device;
	m_uiWidth = uiWidth;
	m_uiHeight = uiHeight;
	m_Format = format;

	BuildResource();
}

void BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, UINT uiDescriptorSize)
{
	m_Blur0CpuSrv = hCpuDescriptor;
	m_Blur0CpuUav = hCpuDescriptor.Offset(1, uiDescriptorSize);
	m_Blur1CpuSrv = hCpuDescriptor.Offset(1, uiDescriptorSize);
	m_Blur1CpuUav = hCpuDescriptor.Offset(1, uiDescriptorSize);

	m_Blur0GpuSrv = hGpuDescriptor;
	m_Blur0GpuUav = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_Blur1GpuSrv = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_Blur1GpuUav = hGpuDescriptor.Offset(1, uiDescriptorSize);

	BuildDescriptors();
}

void BlurFilter::OnResize(UINT uiWidth, UINT uiHeight)
{
	if ((m_uiWidth != uiWidth) || (m_uiHeight != uiHeight)) 
	{
		m_uiWidth = uiWidth;
		m_uiHeight = uiHeight;

		BuildResource();
		BuildDescriptors();
	}
}

void BlurFilter::OnProcess(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* horzBlurPSO, ID3D12PipelineState* vertBlurPSO, ID3D12Resource* input, int blurCount)
{
	auto weights = CalcGaussWeights(2.5f);
	int blurRadius = (int)weights.size() / 2;

	cmdList->SetComputeRootSignature(rootSig);
	cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(input,
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	cmdList->CopyResource(m_BlurMap0.Get(), input);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap1.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	for (int i = 0; i < blurCount; ++i)
	{
		cmdList->SetPipelineState(horzBlurPSO);
		cmdList->SetComputeRootDescriptorTable(1, m_Blur0GpuSrv);
		cmdList->SetComputeRootDescriptorTable(2, m_Blur1GpuUav);

		UINT numGroupX = (UINT)ceil(m_uiWidth / 256.0f);
		cmdList->Dispatch(numGroupX, m_uiHeight, 1);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));

		cmdList->SetPipelineState(vertBlurPSO);
		cmdList->SetComputeRootDescriptorTable(1, m_Blur1GpuSrv);
		cmdList->SetComputeRootDescriptorTable(2, m_Blur0GpuUav);

		UINT numGroupY = (UINT)ceil(m_uiHeight / 256.0f);
		cmdList->Dispatch(m_uiWidth, numGroupY, 1);

		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_BlurMap1.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));
	}
}

ID3D12Resource* BlurFilter::Output()
{
	return m_BlurMap0.Get();
}

std::vector<float> BlurFilter::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// calculate radius
	int blurRadius = (int)ceil(2.0f * sigma);
	assert(blurRadius <= m_MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i) 
	{
		float x = (float)i;

		weights[blurRadius + i] = expf(-x * x / twoSigma2);
		weightSum += weights[blurRadius + i];
	}

	for (int i = 0; i < weights.size(); ++i) 
	{
		weights[i] /= weightSum;
	}
	return weights;
}

void BlurFilter::BuildDescriptors()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	m_d3dDevice->CreateShaderResourceView(m_BlurMap0.Get(), &srvDesc, m_Blur0CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap0.Get(), nullptr, &uavDesc, m_Blur0CpuUav);

	m_d3dDevice->CreateShaderResourceView(m_BlurMap1.Get(), &srvDesc, m_Blur1CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap1.Get(), nullptr, &uavDesc, m_Blur1CpuUav);
}

void BlurFilter::BuildResource()
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
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON, 
		nullptr, 
		IID_PPV_ARGS(&m_BlurMap0)));

	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_BlurMap1)));
}
