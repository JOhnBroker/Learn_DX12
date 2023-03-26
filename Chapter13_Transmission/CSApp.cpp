#include "CSApp.h"

CSApp::CSApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

CSApp::CSApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

CSApp::~CSApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool CSApp::Initialize()
{
	bool bResult = false;

	if (!D3DApp::Initialize()) 
	{
		return bResult;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildCSResource();
	BuildShadersAndInputLayout();
	BuildFrameResources();
	BuildPSOs();

	// execute initialization commands
	HR(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	switch (m_CurrMode)
	{
	case ShowMode::Exercise1:
		DoComputeWork(m_CSExerceise1OutputBuffer.Get(), 1);
		break;
	case ShowMode::Exercise2:
		DoComputeWork(m_CSExerceise2AppendBuffer.Get(), 2);
		break;
	}

	bResult = true;

	return bResult;
}

void CSApp::OnResize()
{
	D3DApp::OnResize();
}

void CSApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Blend demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Exercise1",
			"Exercise2"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::Exercise1;
			}
			else if (curr_mode_item == 1) 
			{
				//// 目前未清楚为什么 AppendStructuredBuffer 无法使用
				//m_CurrMode = ShowMode::Exercise2;
			}
		}
		ImGui::Checkbox("reset calculate", &m_bIsReset);
	}

	ImGui::End();
	ImGui::Render();

	m_CurrFrameResourceIndex = (m_CurrFrameResourceIndex + 1) % g_numFrameResources;
	m_CurrFrameResource = m_FrameResources[m_CurrFrameResourceIndex].get();

	// 等候GPU完成当前帧资源的命令
	if (m_CurrFrameResource->Fence != 0 && m_Fence->GetCompletedValue() < m_CurrFrameResource->Fence) 
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		HR(m_Fence->SetEventOnCompletion(m_CurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}
}

void CSApp::Draw(const GameTimer& timer)
{
	if (m_bIsReset) 
	{
		DoComputeWork(m_CSExerceise1OutputBuffer, 1);
		m_bIsReset = false;
	}
	else
	{
		auto cmdListAlloc = m_CurrFrameResource->CmdListAlloc;

		HR(cmdListAlloc->Reset());

		HR(m_CommandList->Reset(cmdListAlloc.Get(), m_PSOs["opaque"].Get()));

		m_CommandList->RSSetViewports(1, &m_ScreenViewPort);
		m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

		m_CommandList->ClearRenderTargetView(CurrentBackBufferView(), (float*)&m_MainPassCB.FogColor, 0, nullptr);
		m_CommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

		m_CommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

		m_CommandList->SetDescriptorHeaps(1, m_SRVHeap.GetAddressOf());
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());

		m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

		HR(m_CommandList->Close());

		ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
		m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

		HR(m_pSwapChain->Present(0, 0));
		m_CurrentBackBuffer = (m_CurrentBackBuffer + 1) % SwapChainBufferCount;
		m_CurrFrameResource->Fence = ++m_CurrentFence;

		m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
	}
}

void CSApp::DoComputeWork(ComPtr<ID3D12Resource> output, int type)
{
	char szFileName[MAX_PATH] = "";
	struct ResultData
	{
		float norm;
	};

	HR(m_DirectCmdListAlloc->Reset());
	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), m_PSOs["exercise1"].Get()));

	if (type == 1)
	{
		m_CommandList->SetComputeRootSignature(m_RootSignature.Get());
		m_CommandList->SetComputeRootShaderResourceView(0, m_CSExercise1Input->GetGPUVirtualAddress());
		m_CommandList->SetComputeRootUnorderedAccessView(1, output->GetGPUVirtualAddress());
	}
	else if (type == 2)
	{
		ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
		m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

		m_CommandList->SetPipelineState(m_PSOs["exercise2"].Get());

		m_CommandList->SetComputeRootSignature(m_RootSignature.Get());
		m_CommandList->SetComputeRootUnorderedAccessView(2, m_CSExerceise2ConsumeBuffer->GetGPUVirtualAddress());
		m_CommandList->SetComputeRootUnorderedAccessView(3, output->GetGPUVirtualAddress());
	}

	m_CommandList->Dispatch(2, 1, 1);

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		output.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE));

	m_CommandList->CopyResource(m_CSExerceise1ReadbackBuffer.Get(), output.Get());

	m_CommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(
		output.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS));

	HR(m_CommandList->Close());

	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	ResultData* mappedData = nullptr;
	HR(m_CSExerceise1ReadbackBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

	snprintf(szFileName, _countof(szFileName),"results%d.log", type);
	std::ofstream fout(szFileName);

	for (int i = 0; i < 64; ++i)
	{
		fout << m_VecData[i].vec.x << "," 
			<< m_VecData[i].vec.y << "," 
			<< m_VecData[i].vec.z 
			<< "(" << mappedData[i].norm << ")" << std::endl;
	}

	m_CSExerceise1ReadbackBuffer->Unmap(0, nullptr);
}

void CSApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void CSApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void CSApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void CSApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	slotRootParameter[0].InitAsShaderResourceView(0);
	slotRootParameter[1].InitAsUnorderedAccessView(0);
	slotRootParameter[2].InitAsUnorderedAccessView(1);
	slotRootParameter[3].InitAsUnorderedAccessView(2);

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	hReturn = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	HR(hReturn);

	HR(m_pd3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void CSApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};

	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));
}

void CSApp::BuildCSResource()
{
	m_VecData.resize(m_numDataElement);
	for (int i = 0; i < m_numDataElement; ++i)
	{
		m_VecData[i].vec.x = MathHelper::RandF(1.0f, 5.7f);
		m_VecData[i].vec.y = MathHelper::RandF(1.0f, 5.7f);
		m_VecData[i].vec.z = MathHelper::RandF(1.0f, 5.7f);
	}

	UINT64 byteSize = m_VecData.size() * sizeof(Data);

	// Exercise 1
	m_CSExercise1Input = d3dUtil::CreateDefaultBuffer(
		m_pd3dDevice.Get(),m_CommandList.Get(),
		m_VecData.data(), byteSize, m_CSExerceise1UploadBuffer);

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS),
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS , nullptr, IID_PPV_ARGS(&m_CSExerceise1OutputBuffer)));

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK), D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_CSExerceise1ReadbackBuffer)));

	// Exercise 2
	// Create resource
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = byteSize;
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CSExerceise2ConsumeBuffer)));

	HR(m_pd3dDevice->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_CSExerceise2AppendBuffer)));

	// Create View
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements = (UINT)m_VecData.size();
	uavDesc.Buffer.StructureByteStride = sizeof(Data);

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_pd3dDevice->CreateUnorderedAccessView(m_CSExerceise2ConsumeBuffer.Get(), nullptr, &uavDesc, srvHandle);
	srvHandle.Offset(1, m_CBVSRVDescriptorSize);
	m_pd3dDevice->CreateUnorderedAccessView(m_CSExerceise2AppendBuffer.Get(), nullptr, &uavDesc, srvHandle);
}

void CSApp::BuildShadersAndInputLayout()
{
	m_Shaders["exercise1CS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Exercises.hlsl", nullptr, "CS_Exe1", "cs_5_1");
	m_Shaders["exercise2CS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Exercises.hlsl", nullptr, "CS_Exe2", "cs_5_1");
}

void CSApp::BuildPSOs()
{
	// Exercise 1
	D3D12_COMPUTE_PIPELINE_STATE_DESC exercise1PsoDesc = {};
	exercise1PsoDesc.pRootSignature = m_RootSignature.Get();
	exercise1PsoDesc.CS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["exercise1CS"]->GetBufferPointer()),
		m_Shaders["exercise1CS"]->GetBufferSize()
	};
	exercise1PsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	HR(m_pd3dDevice->CreateComputePipelineState(&exercise1PsoDesc, IID_PPV_ARGS(&m_PSOs["exercise1"])));
	
	// 创建Exercise2的PSO会报错，暂时注释
	//// Exercise 2
	//D3D12_COMPUTE_PIPELINE_STATE_DESC exercise2PsoDesc = {};
	//exercise2PsoDesc.pRootSignature = m_RootSignature.Get();
	//exercise2PsoDesc.CS =
	//{
	//	reinterpret_cast<BYTE*>(m_Shaders["exercise2CS"]->GetBufferPointer()),
	//	m_Shaders["exercise2CS"]->GetBufferSize()
	//};
	//exercise2PsoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	//HR(m_pd3dDevice->CreateComputePipelineState(&exercise2PsoDesc, IID_PPV_ARGS(&m_PSOs["exercise2"])));
}

void CSApp::BuildFrameResources()
{
	for (int i = 0; i < g_numFrameResources; ++i) 
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get(), 1));
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> CSApp::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		0.0f,
		8);

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		0.0f,
		8);

	return { pointWrap, pointClamp,
			linearWrap, linearClamp,
			anisotropicWrap ,anisotropicClamp  };
}
