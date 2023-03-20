#include "GpuWaves.h"

GpuWaves::GpuWaves(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, int m, int n, float dx, float dt, float speed, float damping)
{
	m_d3dDevice = device;
	m_NumRows = m;
	m_NumCols = n;

	assert((m * n) % 256 == 0);

	m_VertexCount = m * n;
	m_TriangleCount = (m - 1) * (n - 1) * 2;

	m_fTimeStep = dt;
	m_fSpatialStep = dx;

	float d = damping * dt + 2.0f;
	float e = (speed * speed) * (dt * dt) / (dx * dx);
	m_K[0] = (damping * dt - 2.0f) / d;
	m_K[1] = (4.0f - 8.0f * e) / d;
	m_K[2] = (2.0f * e) / d;

	BuildResource(cmdList);
}

UINT GpuWaves::RowCount() const
{
	return m_NumRows;
}

UINT GpuWaves::ColumnCount() const
{
	return m_NumCols;
}

UINT GpuWaves::VertexCount() const
{
	return m_VertexCount;
}

UINT GpuWaves::TriangleCount() const
{
	return m_TriangleCount;
}

float GpuWaves::Width() const
{
	return m_NumCols * m_fSpatialStep;
}

float GpuWaves::Depth() const
{
	return m_NumRows * m_fSpatialStep;
}

float GpuWaves::SpatialStep() const
{
	return m_fSpatialStep;
}

CD3DX12_GPU_DESCRIPTOR_HANDLE GpuWaves::DisplacementMap() const
{
	return m_CurrSolSrv;
}

UINT GpuWaves::DescriptorCount() const
{
	return m_NumDescriptorCount;
}

void GpuWaves::BuildResource(ID3D12GraphicsCommandList* cmdList)
{
	D3D12_RESOURCE_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(D3D12_RESOURCE_DESC));
	srvDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	srvDesc.Alignment = 0;
	srvDesc.Width = m_NumCols;
	srvDesc.Height = m_NumRows;
	srvDesc.DepthOrArraySize = 1;
	srvDesc.MipLevels = 1;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.SampleDesc.Count = 1;
	srvDesc.SampleDesc.Quality = 0;
	srvDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	srvDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&srvDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_PrevSol)));
	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&srvDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_CurrSol)));
	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&srvDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_NextSol)));

	const UINT num2DSubresources = srvDesc.DepthOrArraySize * srvDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_CurrSol.Get(), 0, num2DSubresources);

	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_PrevUploadBuffer.GetAddressOf())));
	HR(m_d3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_CurrUploadBuffer.GetAddressOf())));

	// 
	std::vector<float> initData(m_NumRows * m_NumCols, 0.0f);
	for (int i = 0; i < initData.size(); ++i) 
	{
		initData[i] = 0.0f;
	}
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData.data();
	subResourceData.RowPitch = m_NumCols * sizeof(float);
	subResourceData.SlicePitch = subResourceData.RowPitch * m_NumRows;

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_PrevSol.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(cmdList, m_PrevSol.Get(), m_PrevUploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_PrevSol.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources(cmdList, m_CurrSol.Get(), m_CurrUploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData);
	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_NextSol.Get(),
		D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

}

void GpuWaves::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE hGpuDescriptor, UINT uiDescriptorSize)
{

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	m_d3dDevice->CreateShaderResourceView(m_PrevSol.Get(), &srvDesc, hCpuDescriptor);
	m_d3dDevice->CreateShaderResourceView(m_CurrSol.Get(), &srvDesc, hCpuDescriptor.Offset(1, uiDescriptorSize));
	m_d3dDevice->CreateShaderResourceView(m_NextSol.Get(), &srvDesc, hCpuDescriptor.Offset(1, uiDescriptorSize));
	m_d3dDevice->CreateUnorderedAccessView(m_PrevSol.Get(), nullptr, &uavDesc, hCpuDescriptor.Offset(1, uiDescriptorSize));
	m_d3dDevice->CreateUnorderedAccessView(m_CurrSol.Get(), nullptr, &uavDesc, hCpuDescriptor.Offset(1, uiDescriptorSize));
	m_d3dDevice->CreateUnorderedAccessView(m_NextSol.Get(), nullptr, &uavDesc, hCpuDescriptor.Offset(1, uiDescriptorSize));

	m_PrevSolSrv = hGpuDescriptor;
	m_CurrSolSrv = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_NextSolSrv = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_PrevSolUav = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_CurrSolUav = hGpuDescriptor.Offset(1, uiDescriptorSize);
	m_NextSolUav = hGpuDescriptor.Offset(1, uiDescriptorSize);
}

void GpuWaves::Update(const GameTimer& gt, ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso)
{
	static float t = 0.0f;

	t += gt.DeltaTime();

	cmdList->SetPipelineState(pso);
	cmdList->SetComputeRootSignature(rootSig);

	if (t >= m_fTimeStep) 
	{
		cmdList->SetComputeRoot32BitConstants(0, 3, &m_K, 0);
		cmdList->SetComputeRootDescriptorTable(1, m_PrevSolUav);
		cmdList->SetComputeRootDescriptorTable(2, m_CurrSolUav);
		cmdList->SetComputeRootDescriptorTable(3, m_NextSolUav);

		UINT numGroupsX = m_NumCols / 16;
		UINT numGroupsY = m_NumRows / 16;
		cmdList->Dispatch(numGroupsX, numGroupsY, 1);

		auto resTemp = m_PrevSol;
		m_PrevSol = m_CurrSol;
		m_CurrSol = m_NextSol;
		m_NextSol = resTemp;

		auto srvTemp = m_PrevSolSrv;
		m_PrevSolSrv = m_CurrSolSrv;
		m_CurrSolSrv = m_NextSolSrv;
		m_NextSolSrv = srvTemp;

		auto uavTemp = m_PrevSolUav;
		m_PrevSolUav = m_CurrSolUav;
		m_CurrSolUav = m_NextSolUav;
		m_NextSolUav = uavTemp;

		t = 0.0f;
		// change state to D3D12_RESOURCE_STATE_GENERIC_READ, for vertex shader to read
		cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ));
	}

}

void GpuWaves::Disturb(ID3D12GraphicsCommandList* cmdList, ID3D12RootSignature* rootSig, ID3D12PipelineState* pso, UINT i, UINT j, float magnitude)
{
	cmdList->SetPipelineState(pso);
	cmdList->SetComputeRootSignature(rootSig);

	UINT disturbIndex[2] = { j,i };
	cmdList->SetComputeRoot32BitConstants(0, 1, &magnitude, 3);
	cmdList->SetComputeRoot32BitConstants(0, 2, disturbIndex, 4);
	cmdList->SetComputeRootDescriptorTable(3, m_CurrSolUav);

	cmdList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(),
		D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	cmdList->Dispatch(1, 1, 1);
}
