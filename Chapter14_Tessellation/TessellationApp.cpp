#include "TessellationApp.h"

TessellationApp::TessellationApp(HINSTANCE hInstance) :D3DApp(hInstance) 
{
}

TessellationApp::TessellationApp(HINSTANCE hInstance, int width, int height)
	: D3DApp(hInstance, width, height) 
{
}

TessellationApp::~TessellationApp()
{
	if (m_pd3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool TessellationApp::Initialize()
{
	bool bResult = false;

	if (!D3DApp::Initialize()) 
	{
		return bResult;
	}

	HR(m_CommandList->Reset(m_DirectCmdListAlloc.Get(), nullptr));

	if (!InitResource()) 
	{
		return bResult;
	}

	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildQuadPatchGeometry();
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
	case ShowMode::Basic:
		break;
	case ShowMode::Sphere:
		break;
	}

	bResult = true;

	return bResult;
}

bool TessellationApp::InitResource()
{
	LoadTexture("white", L"..\\Textures\\white1x1.dds");
	return true;
}

void TessellationApp::OnResize()
{
	D3DApp::OnResize();
}

void TessellationApp::Update(const GameTimer& timer)
{
	// ImGui
	ImGuiIO& io = ImGui::GetIO();
	
	const float dt = timer.DeltaTime();
	if (ImGui::Begin("Tessllation Demo"))
	{
		static int curr_mode_item = static_cast<int>(m_CurrMode);
		const char* mode_strs[] = {
			"Basic",
			"Sphere"
		};
		if (ImGui::Combo("Mode", &curr_mode_item, mode_strs, ARRAYSIZE(mode_strs)))
		{
			if (curr_mode_item == 0)
			{
				m_CurrMode = ShowMode::Basic;
			}
			else if (curr_mode_item == 1) 
			{
				m_CurrMode = ShowMode::Sphere;
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

void TessellationApp::Draw(const GameTimer& timer)
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

void TessellationApp::OnMouseMove(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
}

void TessellationApp::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void TessellationApp::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_LastMousePos.x = x;
	m_LastMousePos.y = y;
	SetCapture(m_hMainWnd);
}

void TessellationApp::UpdateCamera(const GameTimer& gt)
{
}

void TessellationApp::UpdateObjects(const GameTimer& gt)
{
}

void TessellationApp::UpdateMaterials(const GameTimer& gt)
{
}

void TessellationApp::UpdateMainPass(const GameTimer& gt)
{
}

void TessellationApp::BuildRootSignature()
{
	HRESULT hReturn = E_FAIL;
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);

	auto staticSamplers = GetStaticSamplers();

	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		_countof(slotRootParameter), slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(), 
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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

void TessellationApp::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};

	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	HR(m_pd3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	auto whiteTex = m_Textures["white"]->m_Resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = whiteTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	m_pd3dDevice->CreateShaderResourceView(whiteTex.Get(), &srvDesc, hDescriptor);
}

void TessellationApp::BuildShadersAndInputLayout()
{
	m_Shaders["basicVS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Tesselator.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["basicHS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Tesselator.hlsl", nullptr, "HS", "hs_5_1");
	m_Shaders["basicDS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Tesselator.hlsl", nullptr, "DS", "ds_5_1");
	m_Shaders["basicPS"]	= d3dUtil::CompileShader(L"..\\Shader\\Chapter13\\Tesselator.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
}

void TessellationApp::BuildQuadPatchGeometry()
{
}

void TessellationApp::BuildPSOs()
{
}

void TessellationApp::BuildFrameResources()
{
	for (int i = 0; i < g_numFrameResources; ++i) 
	{
		m_FrameResources.push_back(std::make_unique<FrameResource>(m_pd3dDevice.Get()
			, 1, m_AllRitems.size(), m_Materials.size()));
	}
}

void TessellationApp::BuildMaterials()
{
}

void TessellationApp::BuildRenderItems()
{
}

void TessellationApp::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<RenderItem*>& ritems)
{
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, STATICSAMPLERCOUNT> TessellationApp::GetStaticSamplers()
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

void TessellationApp::LoadTexture(std::string name, std::wstring filename)
{
	auto texture = std::make_unique<Texture>();

	texture->m_Name = name;
	texture->m_Filename = filename;
	HR(DirectX::CreateDDSTextureFromFile12(m_pd3dDevice.Get(), m_CommandList.Get(),
		texture->m_Filename.c_str(), texture->m_Resource, texture->m_UploadHeap));
	m_Textures[name] = std::move(texture);

}
